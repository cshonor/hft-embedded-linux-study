fn main() {
    paths_demo::eat_at_restaurant();
    println!("ok: crate:: / 相对 / super:: / self:: 路径 + pub 字段");

    let mut meal = paths_demo::Breakfast::summer("Rye");
    meal.toast = String::from("Wheat");
    println!("toast = {} (pub 字段可改)", meal.toast);
    println!("fruit = {} (私有字段经 getter)", meal.seasonal_fruit());

    let _ = paths_demo::Appetizer::Soup;
}
