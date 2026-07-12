#[derive(Debug)]
struct Rectangle {
    width: u32,
    height: u32,
}

impl Rectangle {
    fn area(&self) -> u32 {
        self.width * self.height
    }
    fn new(width: u32, height: u32) -> Self {
        Self { width, height }
    }
    fn can_hold(&self, other: &Rectangle) -> bool {
        other.height <= self.height && other.width <= self.width
    }
    fn square(size: u32) -> Self {
        Self::new(size, size)
    }
}

fn main() {
    let rect_1 = Rectangle {
        width: 30,
        height: 50,
    };
    let rect_2 = Rectangle::new(40, 60);
    let rect_3 = Rectangle::square(50);
    println!("{:?}", rect_1);
    println!("{:#?}", rect_1);

    dbg!(&rect_1);

    let sq_1 = rect_1.area();
    println!("area for rect_1 = {}", sq_1);

    let sq_3 = rect_3.area();
    println!("area for rect_3 = {}", sq_3);

    println!("rect_1 can hold rect_2 is {}", rect_1.can_hold(&rect_2));
    println!("rect_2 can hold rect_1 is {}", rect_2.can_hold(&rect_1));
}
