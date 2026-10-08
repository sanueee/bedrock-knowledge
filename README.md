# bedrock-knowledge

место обучения: вузовские предметы, задания по личной стратегии (системное программирование и безопасность — c, linux, сети, rust) и свои задачки. код и конспекты к нему.

## структура

```
knowledge-base/
  university/          вуз, по семестрам
    2-semester/ds/     структуры данных: theory/ (конспекты) + practice/ (реализации на c)
    3-semester/os/     операционные системы (pintos)
  self-study/          личная стратегия
    topics/            конспекты: c, linux, networking, security, rust, ...
    roadmap/           текущая позиция, архив тем, журнал заданий
    strategy/          план блоков
  pt-course/           материалы курса pt start 2026
self-practice/
  networking/          задания на c — сокеты, dns, http, tls, raw sockets, icmp, сканер портов
  linux/               процессы, fd, pipes, сигналы, потоки, /proc
  rust/                по мере чтения the rust book
  leetcode/            отдельный трек
  ctf/                 picoctf: сессии и райтапы
.claude/               правила и скиллы для claude code
```

## как это устроено

задание по стратегии проходит один цикл: постановка (`task-*.md`) → код → ревью с разбором ошибок → конспект в `self-study/topics/`. в task-файле остаётся разбор: что не получилось с первого раза и почему.

вузовский предмет живёт целиком в своей папке: `theory/` — конспекты лекций и тем, `practice/` — лабы и задачи.
