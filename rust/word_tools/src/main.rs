fn main() {
    let text = "   a bb ccc ";
    println!("first   : {}", first_word(text));
    println!("count   : {}", count_words(text));
    println!("longest : {}", longest_word(text));
}

fn first_word(s: &str) -> &str {
    let bytes = s.as_bytes();
    for (i, &byte) in bytes.iter().enumerate() {
        if byte == b' ' {
            return &s[0..i];
        }
    }
    &s[..]
}

fn count_words(s: &str) -> usize {
    let bytes = s.as_bytes();
    let mut counter: usize = 0;
    let mut in_word: bool = false;
    for &byte in bytes.iter() {
        if byte != b' ' && in_word == false {
            counter += 1;
            in_word = true;
        }
        if byte == b' ' {
            in_word = false;
        }
    }
    counter
}

fn longest_word(s: &str) -> &str {
    let mut max_len = 0;
    let mut curr_len = 0;
    let mut result: &str = "";
    let bytes = s.as_bytes();
    for (i, &byte) in bytes.iter().enumerate() { // abb abdc
        if byte != b' ' {
            curr_len += 1;
        }
        if curr_len > max_len {
            max_len = curr_len;
            result = &s[i+1-curr_len..=i];
        }
        if byte == b' ' {
            curr_len = 0;
        }
    }
    result
}