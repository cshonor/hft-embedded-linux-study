use std::thread;

pub fn main() {
    println!("Hello, world!");
    let  t1= thread::spawn(f);
    let  t2= thread::spawn(f);
    //t1.join().unwrap();和t2.join().unwrap();
    // 用于等待两个新线程执行完毕。join方法会阻塞当前线程（这里是主线程），直到被调用的线程（t1和t2对应的线程）执行结束。
    t1.join().unwrap();
    t2.join().unwrap();
}

fn f()  {
 
  println!("Hello from the thread!");
    let id=thread::current().id();
    println!("Thread id is: {:?}", id);

}