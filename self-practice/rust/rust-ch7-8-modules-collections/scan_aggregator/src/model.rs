#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub enum PortState {
    Open,
    Closed,
    Filtered
}

impl PortState {
    pub fn describe(&self) -> &str {
        match self {
            PortState::Open => "opened",
            PortState::Closed => "closed",
            PortState::Filtered => "filtered"
        } 
    }
}

pub struct ScanResult {
    pub host: String,
    pub port: u16,
    pub state: PortState,
}