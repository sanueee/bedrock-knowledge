mod model;
mod report;

use crate::report::write_report;
use model::{ScanResult, PortState};


fn main() {
    let results = vec![
        ScanResult { host: String::from("scanme.local"), port: 22,   state: PortState::Open },
        ScanResult { host: String::from("scanme.local"), port: 80,   state: PortState::Open },
        ScanResult { host: String::from("scanme.local"), port: 443,  state: PortState::Open },
        ScanResult { host: String::from("scanme.local"), port: 23,   state: PortState::Closed },
        ScanResult { host: String::from("db.local"),     port: 5432, state: PortState::Open },
        ScanResult { host: String::from("db.local"),     port: 3306, state: PortState::Closed },
        ScanResult { host: String::from("db.local"),     port: 8080, state: PortState::Filtered },
    ];

    println!("{}", write_report(&results));
}