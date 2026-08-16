fn prediction_iter(buffer: &[i32], coefficients: &[i64; 12], qlp_shift: i16, i: usize) -> i32 {
    let prediction: i64 = coefficients
        .iter()
        .zip(&buffer[i - 12..i])
        .map(|(&c, &s)| c * s as i64)
        .sum::<i64>()
        >> qlp_shift;

    prediction as i32 + buffer[i]
}

fn prediction_loop(buffer: &[i32], coefficients: &[i64; 12], qlp_shift: i16, i: usize) -> i32 {
    let mut acc: i64 = 0;
    for j in 0..12 {
        acc += coefficients[j] * buffer[i - 12 + j] as i64;
    }
    let prediction = acc >> qlp_shift;
    prediction as i32 + buffer[i]
}

fn main() {
    let coefficients: [i64; 12] = [2, -1, 3, 0, 1, -2, 4, 1, 0, -1, 2, 1];
    let qlp_shift: i16 = 2;

    // buffer 至少 13 个元素，且 i 从 12 开始才有“前 12 个样本窗口”
    let mut buffer: Vec<i32> = (0..32).map(|x| x * 3 - 10).collect();

    for i in 12..buffer.len() {
        let a = prediction_iter(&buffer, &coefficients, qlp_shift, i);
        let b = prediction_loop(&buffer, &coefficients, qlp_shift, i);
        assert_eq!(a, b, "mismatch at i={i}");

        // 模拟把结果写回（像解码器一样会更新 buffer）
        buffer[i] = a;
    }

    println!("Iterator-chain and loop versions match.");
}

