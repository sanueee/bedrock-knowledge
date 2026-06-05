use crate::model::{ScanResult, PortState};
use std::collections::HashMap;

fn count_ports(results: &[ScanResult]) -> usize {
    let mut n = 0;
    for r in results {
        if r.state == PortState::Open {
            n += 1;
        }
    }
    n
}

fn group_open_ports(results: &[ScanResult]) -> HashMap<String, Vec<u16>> {
    let mut map = HashMap::new();
    for r in results {
        if r.state == PortState::Open {
            map.entry(r.host.clone()).or_insert_with(Vec::new).push(r.port);
        }
    }
    map
}

fn count_by_state(results: &[ScanResult]) -> HashMap<PortState, u32> {
    let mut map = HashMap::new();
    for r in results {
        let counter: &mut u32 = map.entry(r.state).or_insert(0);
        *counter += 1;
    }
    map
}

pub fn write_report(results: &[ScanResult]) -> String {
    let open_count = count_ports(results);
    let groups = group_open_ports(results);

    let mut report = String::new();
    report.push_str("=== Scan summary ===\n");
    report.push_str(&format!("Hosts scanned: {}\n", groups.len()));
    report.push_str(&format!("Open ports total: {}\n", open_count));
    report.push('\n');
    for (host, ports) in &groups { // здесь можно и с & и без него верно ? с & объект groups дропнется после завершения функции а без него дропнется после цикла этого
        report.push_str(&format!("{}: ", host));
        let mut first = true;
        for port in ports {
            if !first {
                report.push_str(", ");
            }
            report.push_str(&port.to_string());
            first = false;
        }
        report.push('\n');
    }
    report.push('\n');

    let filtered = count_by_state(results);
    report.push_str("State histogram: ");
    for (state, count) in &filtered {
        report.push_str(&format!("{}={} ", state.describe(), count));
    }

    report
}