//! Blanket impl 转发 — 供 03-1 文档引用

pub trait MyTrait {
    fn work(&self);
}

pub struct Foo;

impl MyTrait for Foo {
    fn work(&self) {
        println!("ok");
    }
}

impl<T: MyTrait + ?Sized> MyTrait for &T {
    fn work(&self) {
        (*self).work()
    }
}

impl<T: MyTrait + ?Sized> MyTrait for &mut T {
    fn work(&self) {
        (**self).work()
    }
}

impl<T: MyTrait + ?Sized> MyTrait for Box<T> {
    fn work(&self) {
        (**self).work()
    }
}

impl<T: MyTrait + ?Sized> MyTrait for std::sync::Arc<T> {
    fn work(&self) {
        (**self).work()
    }
}

impl<T: MyTrait + ?Sized> MyTrait for std::rc::Rc<T> {
    fn work(&self) {
        (**self).work()
    }
}

pub trait Count {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool {
        self.len() == 0
    }
}

pub struct Items(pub Vec<i32>);

impl Count for Items {
    fn len(&self) -> usize {
        self.0.len()
    }
}

impl<T: Count + ?Sized> Count for &T {
    fn len(&self) -> usize {
        (**self).len()
    }
}
