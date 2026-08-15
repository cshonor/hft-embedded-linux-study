use hello_macro::HelloMacro;
use hello_macro_derive::HelloMacro as HelloMacroDerive;

#[derive(HelloMacroDerive)]
struct Pancakes;

fn main() {
    Pancakes::hello_macro();
}

