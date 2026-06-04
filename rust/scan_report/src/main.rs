
enum PortState {
    Open(String),                 // tuple-like вариант: String — имя сервиса ("ssh", "http")
    Closed,                       // unit-like вариант: данных нет
    Filtered { reason: String },  // struct-like вариант: именованное поле
}

struct PortResult {
    port: u16,
    state: PortState,
}

impl PortState {
    fn describe(&self) -> String {
        match self {
            PortState::Open(service) => {
                format!("открыт (сервис: {})", service)
            },
            PortState::Closed => {
                String::from("закрыт")
            },
            PortState::Filtered { reason } => {
                format!("фильтруется: {}", reason)
            }
        } 
    }
}

fn find_port(results: &[PortResult], port: u16) -> Option<&PortResult> {
    for result in results {
        if result.port == port {
            return Some(result);
        }
    }
    None
}

fn main() {
    let results = vec![
        PortResult { port: 22,  state: PortState::Open(String::from("ssh")) },
        PortResult { port: 80,  state: PortState::Open(String::from("http")) },
        PortResult { port: 139, state: PortState::Filtered { reason: String::from("no response") } },
        PortResult { port: 443, state: PortState::Closed },
    ];

    match find_port(&results, 80) {
        Some(r) => println!("port 80: {}", r.state.describe()),
        None => println!("port 80 not scanned")
    }

    if let Some(r) = find_port(&results, 8080) {
        println!("port 8080: {}", r.state.describe())
    }

    for result in &results {
        println!("port {}: {}", result.port, result.state.describe());
    }
}
