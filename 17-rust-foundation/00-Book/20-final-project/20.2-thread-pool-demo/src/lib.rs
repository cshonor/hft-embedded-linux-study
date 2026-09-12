//! The Book 第 20 章：线程池 + 优雅停机。
//!
//! 三个关键点（详见 `../20.2-多线程与线程池.md` 与 `../20.3-优雅停机与清理.md`）：
//! 1. `Receiver` 不是 `Sync` → 共享必须 `Arc<Mutex<Receiver>>`
//! 2. `MutexGuard` 必须在 `recv()` 之后立刻释放，否则池子退化成单线程
//! 3. `Drop` 里只能**发信号**（`drop(sender)`），不能等；收工靠 `join`

use std::sync::{mpsc, Arc, Mutex};
use std::thread;

/// 任务类型：`FnOnce` 只执行一次，`Send` 能跨线程，`'static` 不借用局部变量。
/// 大小未知，所以装箱成 trait object（ch19 `?Sized` 的实际用处）。
type Job = Box<dyn FnOnce() + Send + 'static>;

pub struct ThreadPool {
    workers: Vec<Worker>,
    sender: Option<mpsc::Sender<Job>>,
}

#[derive(Debug)]
pub struct PoolBuildError;

impl ThreadPool {
    /// 建池。`size == 0` 直接报错而不是 panic —— 库代码不该替调用者决定。
    pub fn build(size: usize) -> Result<ThreadPool, PoolBuildError> {
        if size == 0 {
            return Err(PoolBuildError);
        }

        let (sender, receiver) = mpsc::channel();
        // 关键：Receiver 不是 Sync，包 Mutex 提供互斥、包 Arc 提供共享。
        let receiver = Arc::new(Mutex::new(receiver));

        let workers = (0..size)
            .map(|id| Worker::new(id, Arc::clone(&receiver)))
            .collect();

        Ok(ThreadPool {
            workers,
            sender: Some(sender),
        })
    }

    pub fn execute<F>(&self, f: F) -> Result<(), mpsc::SendError<Job>>
    where
        F: FnOnce() + Send + 'static,
    {
        let Some(sender) = self.sender.as_ref() else {
            let dummy: Job = Box::new(|| {});
            return Err(mpsc::SendError(dummy));
        };
        sender.send(Box::new(f))
    }

    pub fn worker_count(&self) -> usize {
        self.workers.len()
    }
}

impl Drop for ThreadPool {
    fn drop(&mut self) {
        // ① 发停机信号：sender 一 drop，worker 的 recv() 立刻返回 Err。
        //    必须写在 join 之前 —— 顺序反了就是死锁。
        drop(self.sender.take());

        // ② 收工：等每个 worker 真正退出。
        //    Drop::drop 只有 &mut self，取所有权靠 Option::take()。
        for worker in &mut self.workers {
            if let Some(handle) = worker.thread.take() {
                if handle.join().is_err() {
                    eprintln!("worker {} panicked", worker.id);
                }
            }
        }
    }
}

struct Worker {
    id: usize,
    thread: Option<thread::JoinHandle<()>>,
}

impl Worker {
    fn new(id: usize, receiver: Arc<Mutex<mpsc::Receiver<Job>>>) -> Worker {
        let thread = thread::spawn(move || loop {
            // 注意这对大括号：让 MutexGuard 在 recv() 之后立刻释放。
            // 若写成 `while let Ok(job) = receiver.lock().unwrap().recv()`，
            // guard 会活到整个语句结束（含任务执行），锁粒度从「一次 recv」
            // 变成「整个任务」，线程池直接退化成单线程。
            let job = {
                let lock = receiver.lock().unwrap();
                lock.recv()
            };

            match job {
                Ok(job) => job(),
                Err(_) => break, // sender 已 drop → 停机
            }
        });

        Worker {
            id,
            thread: Some(thread),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::time::{Duration, Instant};

    #[test]
    fn zero_sized_pool_is_rejected() {
        assert!(ThreadPool::build(0).is_err());
    }

    #[test]
    fn every_submitted_job_runs() {
        let pool = ThreadPool::build(4).unwrap();
        let counter = Arc::new(AtomicUsize::new(0));

        for _ in 0..50 {
            let c = Arc::clone(&counter);
            pool.execute(move || {
                c.fetch_add(1, Ordering::SeqCst);
            })
            .unwrap();
        }

        // Drop 会 drop(sender) 并 join 所有 worker，
        // 所以从 drop 返回时全部任务必然已执行完。
        drop(pool);
        assert_eq!(counter.load(Ordering::SeqCst), 50);
    }

    #[test]
    fn jobs_actually_run_in_parallel() {
        // 4 个各睡 200ms 的任务，4 个 worker：
        // 若退化成单线程需要 800ms，并行则约 200ms。
        let pool = ThreadPool::build(4).unwrap();
        let start = Instant::now();

        for _ in 0..4 {
            pool.execute(|| thread::sleep(Duration::from_millis(200)))
                .unwrap();
        }
        drop(pool);

        let elapsed = start.elapsed();
        assert!(
            elapsed < Duration::from_millis(600),
            "4 个 200ms 任务在 4 线程池里耗时 {elapsed:?}，说明锁粒度有问题（退化成串行了）"
        );
    }

    #[test]
    fn graceful_shutdown_returns_even_with_slow_jobs() {
        let pool = ThreadPool::build(2).unwrap();
        for _ in 0..4 {
            pool.execute(|| thread::sleep(Duration::from_millis(50)))
                .unwrap();
        }
        // 如果 Drop 里顺序写反（先 join 再 drop sender），这里会永远卡住。
        drop(pool);
    }
}
