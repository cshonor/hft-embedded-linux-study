use crate::market::MockFeed;
use crate::oms::{OrderState, Oms};
use crate::risk::RiskGate;
use crate::strategy::{DemoMomentum, Strategy};

pub async fn run_demo() {
    let mut strategy = DemoMomentum::new(1.0);
    let risk = RiskGate::new(10.0);
    let mut oms = Oms::new();
    let mut position = 0.0_f64;

    for bar in MockFeed::sample_bars() {
        // 显式读 bar 的各字段（而非 Debug 整个结构体），
        // 否则 open/high/low/volume 写了却没人读，等于死数据。
        tracing::info!(
            symbol = bar.symbol,
            close = bar.close,
            range = bar.range(),
            notional = bar.notional(),
            bullish = bar.is_bullish(),
            "tick"
        );

        let Some(signal) = strategy.on_bar(&bar) else {
            tracing::debug!(symbol = bar.symbol, "no signal");
            continue;
        };

        if !risk.check(&signal) {
            tracing::warn!(?signal, "risk rejected");
            continue;
        }

        let order = oms.submit(signal);

        // 真正读取 order 的字段：只有 Accepted 才更新持仓，
        // 否则 state / signal 两个字段写了却没人读（等于死代码）。
        if order.state == OrderState::Accepted {
            position += order.signal.signed_qty();
            tracing::info!(
                order_id = order.id,
                ?position,
                strategy_pos = strategy.position(),
                "position updated"
            );
        } else {
            tracing::warn!(order_id = order.id, ?order.state, "order not accepted");
        }
    }

    tracing::info!(%position, "run_demo finished");
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::strategy::Signal;

    #[tokio::test]
    async fn demo_run_ends_flat_after_buy_and_sell() {
        let mut strategy = DemoMomentum::new(1.0);
        let mut oms = Oms::new();
        let mut position = 0.0_f64;
        let (mut buys, mut sells) = (0usize, 0usize);

        for bar in MockFeed::sample_bars() {
            if let Some(signal) = strategy.on_bar(&bar) {
                match signal {
                    Signal::Buy { .. } => buys += 1,
                    Signal::Sell { .. } => sells += 1,
                }
                let order = oms.submit(signal);
                if order.state == OrderState::Accepted {
                    position += order.signal.signed_qty();
                }
            }
        }

        // 不能只断言「最终持仓为 0」——策略一个信号都不出时它同样是 0，断言会假通过。
        // （这个坑真实发生过：prev_close 没落盘导致策略全哑，测试却显示绿色。）
        assert!(buys > 0, "样本 bar 应至少触发一次 Buy");
        assert!(sells > 0, "样本 bar 应至少触发一次 Sell（否则等于没实现卖出）");
        assert_eq!(position, 0.0, "demo 走完应回到空仓");
    }
}
