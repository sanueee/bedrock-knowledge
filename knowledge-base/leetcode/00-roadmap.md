# LeetCode Roadmap — план и прогресс

Отдельный трек, **вне** учебной стратегии СЗИ. Цель: паттерны для собеседований + образ мышления. Языки: **Python** (основной), **Rust** (второй проход, факультативно).

## Как это работает

- **Источник плана и прогресса** — этот файл. Порядок разделов и задач внутри них неслучаен: каждый следующий раздел опирается на предыдущие.
- **Прогресс определяется отметкой `[x]`** в строках плана ниже: `[x]` = решено и записано, `[ ]` = нет. Это единственный источник истины — скилл `leetcode` ищет первую `[ ]` и советует её.
- **Папка = хранилище, не статус.** Папка `<slug>/` создаётся уже при выдаче задания (туда кладётся заготовка `solution.py`, которая открывается в редакторе). Наличие папки **не** значит «решено» — задача может быть «в работе»: папка есть, но `[ ]`. Отметку `[x]` ставит скилл только при записи готового решения.
- Папки на `_` (напр. `_TEMPLATE/`) — служебные, не задачи.
- Одна задача = одна папка = `solution.py` (или `.rs`) + `notes.md` (паттерн, теги, идея, сложность).

## Правила прохождения

- Порядок 1→18 по разделам. Разделы 9 (Tries), 12 (Advanced Graphs), 17 (Math) можно отложить — встречаются реже.
- ~8–12 задач на раздел: сначала Easy для механики, потом Medium (собесы — это Medium).
- Застрял 25–30 мин → читаешь разбор, разбираешься, **решаешь заново без подсказки через день**.
- Считаем паттерны, а не задачи: цель — узнавать «это sliding window», а не «нарешал 300».

---

## План по разделам

Формат: `[ ] slug — Название (сложность) · #тег`. `slug` = кусок URL leetcode.com/problems/**slug**/. Отметка `[x]` = решено — источник истины прогресса, ставит скилл при записи решения. `[ ]` = не решено (в т.ч. «в работе», когда папка уже есть).

### 1. Arrays & Hashing
- [x] `two-sum` — Two Sum (Easy) · #hashmap
- [x] `contains-duplicate` — Contains Duplicate (Easy) · #hashset
- [x] `valid-anagram` — Valid Anagram (Easy) · #hashmap #counting
- [x] `group-anagrams` — Group Anagrams (Medium) · #hashmap
- [x] `top-k-frequent-elements` — Top K Frequent Elements (Medium) · #hashmap #bucket-sort
- [x] `product-of-array-except-self` — Product of Array Except Self (Medium) · #prefix
- [ ] `valid-sudoku` — Valid Sudoku (Medium) · #hashset
- [ ] `longest-consecutive-sequence` — Longest Consecutive Sequence (Medium) · #hashset

### 2. Two Pointers
- [ ] `valid-palindrome` — Valid Palindrome (Easy) · #two-pointers
- [ ] `two-sum-ii-input-array-is-sorted` — Two Sum II (Medium) · #two-pointers
- [ ] `3sum` — 3Sum (Medium) · #two-pointers #sort
- [ ] `container-with-most-water` — Container With Most Water (Medium) · #two-pointers #greedy
- [ ] `trapping-rain-water` — Trapping Rain Water (Hard) · #two-pointers

### 3. Sliding Window
- [ ] `best-time-to-buy-and-sell-stock` — Best Time to Buy/Sell Stock (Easy) · #sliding-window
- [ ] `longest-substring-without-repeating-characters` — Longest Substring Without Repeating (Medium) · #sliding-window #hashset
- [ ] `longest-repeating-character-replacement` — Longest Repeating Char Replacement (Medium) · #sliding-window
- [ ] `permutation-in-string` — Permutation in String (Medium) · #sliding-window
- [ ] `minimum-window-substring` — Minimum Window Substring (Hard) · #sliding-window

### 4. Stack
- [ ] `valid-parentheses` — Valid Parentheses (Easy) · #stack
- [ ] `min-stack` — Min Stack (Medium) · #stack #design
- [ ] `evaluate-reverse-polish-notation` — Evaluate RPN (Medium) · #stack
- [ ] `generate-parentheses` — Generate Parentheses (Medium) · #stack #backtracking
- [ ] `daily-temperatures` — Daily Temperatures (Medium) · #monotonic-stack
- [ ] `car-fleet` — Car Fleet (Medium) · #stack #sort
- [ ] `largest-rectangle-in-histogram` — Largest Rectangle in Histogram (Hard) · #monotonic-stack

### 5. Binary Search
- [ ] `binary-search` — Binary Search (Easy) · #binary-search
- [ ] `search-a-2d-matrix` — Search a 2D Matrix (Medium) · #binary-search
- [ ] `koko-eating-bananas` — Koko Eating Bananas (Medium) · #binary-search-on-answer
- [ ] `find-minimum-in-rotated-sorted-array` — Find Min in Rotated Sorted Array (Medium) · #binary-search
- [ ] `search-in-rotated-sorted-array` — Search in Rotated Sorted Array (Medium) · #binary-search
- [ ] `time-based-key-value-store` — Time Based Key-Value Store (Medium) · #binary-search #design
- [ ] `median-of-two-sorted-arrays` — Median of Two Sorted Arrays (Hard) · #binary-search

### 6. Linked List
- [ ] `reverse-linked-list` — Reverse Linked List (Easy) · #linked-list
- [ ] `merge-two-sorted-lists` — Merge Two Sorted Lists (Easy) · #linked-list
- [ ] `linked-list-cycle` — Linked List Cycle (Easy) · #fast-slow
- [ ] `reorder-list` — Reorder List (Medium) · #linked-list #fast-slow
- [ ] `remove-nth-node-from-end-of-list` — Remove Nth Node From End (Medium) · #two-pointers
- [ ] `copy-list-with-random-pointer` — Copy List with Random Pointer (Medium) · #hashmap
- [ ] `add-two-numbers` — Add Two Numbers (Medium) · #linked-list
- [ ] `find-the-duplicate-number` — Find the Duplicate Number (Medium) · #fast-slow
- [ ] `lru-cache` — LRU Cache (Medium) · #design #hashmap
- [ ] `merge-k-sorted-lists` — Merge k Sorted Lists (Hard) · #heap #linked-list

### 7. Trees
- [ ] `invert-binary-tree` — Invert Binary Tree (Easy) · #tree #dfs
- [ ] `maximum-depth-of-binary-tree` — Max Depth of Binary Tree (Easy) · #tree #dfs
- [ ] `diameter-of-binary-tree` — Diameter of Binary Tree (Easy) · #tree #dfs
- [ ] `balanced-binary-tree` — Balanced Binary Tree (Easy) · #tree #dfs
- [ ] `same-tree` — Same Tree (Easy) · #tree #dfs
- [ ] `subtree-of-another-tree` — Subtree of Another Tree (Easy) · #tree #dfs
- [ ] `lowest-common-ancestor-of-a-binary-search-tree` — LCA of BST (Medium) · #bst
- [ ] `binary-tree-level-order-traversal` — Level Order Traversal (Medium) · #tree #bfs
- [ ] `binary-tree-right-side-view` — Right Side View (Medium) · #tree #bfs
- [ ] `count-good-nodes-in-binary-tree` — Count Good Nodes (Medium) · #tree #dfs
- [ ] `validate-binary-search-tree` — Validate BST (Medium) · #bst #dfs
- [ ] `kth-smallest-element-in-a-bst` — Kth Smallest in BST (Medium) · #bst #inorder
- [ ] `construct-binary-tree-from-preorder-and-inorder-traversal` — Build Tree from Pre/Inorder (Medium) · #tree #recursion
- [ ] `binary-tree-maximum-path-sum` — Max Path Sum (Hard) · #tree #dfs
- [ ] `serialize-and-deserialize-binary-tree` — Serialize/Deserialize Tree (Hard) · #tree #bfs

### 8. Heap / Priority Queue
- [ ] `kth-largest-element-in-a-stream` — Kth Largest in Stream (Easy) · #heap #design
- [ ] `last-stone-weight` — Last Stone Weight (Easy) · #heap
- [ ] `k-closest-points-to-origin` — K Closest Points to Origin (Medium) · #heap
- [ ] `kth-largest-element-in-an-array` — Kth Largest in Array (Medium) · #heap #quickselect
- [ ] `task-scheduler` — Task Scheduler (Medium) · #heap #greedy
- [ ] `design-twitter` — Design Twitter (Medium) · #heap #design
- [ ] `find-median-from-data-stream` — Find Median from Data Stream (Hard) · #two-heaps

### 9. Tries
- [ ] `implement-trie-prefix-tree` — Implement Trie (Medium) · #trie #design
- [ ] `design-add-and-search-words-data-structure` — Add and Search Word (Medium) · #trie #dfs
- [ ] `word-search-ii` — Word Search II (Hard) · #trie #backtracking

### 10. Backtracking
- [ ] `subsets` — Subsets (Medium) · #backtracking
- [ ] `combination-sum` — Combination Sum (Medium) · #backtracking
- [ ] `permutations` — Permutations (Medium) · #backtracking
- [ ] `subsets-ii` — Subsets II (Medium) · #backtracking
- [ ] `combination-sum-ii` — Combination Sum II (Medium) · #backtracking
- [ ] `word-search` — Word Search (Medium) · #backtracking #grid
- [ ] `palindrome-partitioning` — Palindrome Partitioning (Medium) · #backtracking
- [ ] `letter-combinations-of-a-phone-number` — Letter Combinations (Medium) · #backtracking
- [ ] `n-queens` — N-Queens (Hard) · #backtracking

### 11. Graphs
- [ ] `number-of-islands` — Number of Islands (Medium) · #dfs #bfs #grid
- [ ] `clone-graph` — Clone Graph (Medium) · #dfs #hashmap
- [ ] `max-area-of-island` — Max Area of Island (Medium) · #dfs #grid
- [ ] `pacific-atlantic-water-flow` — Pacific Atlantic Water Flow (Medium) · #dfs #grid
- [ ] `surrounded-regions` — Surrounded Regions (Medium) · #dfs #grid
- [ ] `rotting-oranges` — Rotting Oranges (Medium) · #bfs #grid
- [ ] `walls-and-gates` — Walls and Gates (Medium) · #bfs #grid
- [ ] `course-schedule` — Course Schedule (Medium) · #topological-sort
- [ ] `course-schedule-ii` — Course Schedule II (Medium) · #topological-sort
- [ ] `redundant-connection` — Redundant Connection (Medium) · #union-find
- [ ] `number-of-connected-components-in-an-undirected-graph` — Connected Components (Medium) · #union-find
- [ ] `graph-valid-tree` — Graph Valid Tree (Medium) · #union-find

### 12. Advanced Graphs
- [ ] `network-delay-time` — Network Delay Time (Medium) · #dijkstra
- [ ] `min-cost-to-connect-all-points` — Min Cost to Connect Points (Medium) · #mst #prim
- [ ] `cheapest-flights-within-k-stops` — Cheapest Flights K Stops (Medium) · #bellman-ford
- [ ] `reconstruct-itinerary` — Reconstruct Itinerary (Hard) · #eulerian #dfs
- [ ] `swim-in-rising-water` — Swim in Rising Water (Hard) · #dijkstra #binary-search
- [ ] `word-ladder` — Word Ladder (Hard) · #bfs

### 13. 1-D Dynamic Programming
- [ ] `climbing-stairs` — Climbing Stairs (Easy) · #dp
- [ ] `min-cost-climbing-stairs` — Min Cost Climbing Stairs (Easy) · #dp
- [ ] `house-robber` — House Robber (Medium) · #dp
- [ ] `house-robber-ii` — House Robber II (Medium) · #dp
- [ ] `longest-palindromic-substring` — Longest Palindromic Substring (Medium) · #dp #two-pointers
- [ ] `palindromic-substrings` — Palindromic Substrings (Medium) · #dp
- [ ] `decode-ways` — Decode Ways (Medium) · #dp
- [ ] `coin-change` — Coin Change (Medium) · #dp
- [ ] `maximum-product-subarray` — Maximum Product Subarray (Medium) · #dp
- [ ] `word-break` — Word Break (Medium) · #dp
- [ ] `longest-increasing-subsequence` — Longest Increasing Subsequence (Medium) · #dp
- [ ] `partition-equal-subset-sum` — Partition Equal Subset Sum (Medium) · #dp #knapsack

### 14. 2-D Dynamic Programming
- [ ] `unique-paths` — Unique Paths (Medium) · #dp #grid
- [ ] `longest-common-subsequence` — Longest Common Subsequence (Medium) · #dp
- [ ] `best-time-to-buy-and-sell-stock-with-cooldown` — Buy/Sell with Cooldown (Medium) · #dp
- [ ] `coin-change-ii` — Coin Change II (Medium) · #dp #knapsack
- [ ] `target-sum` — Target Sum (Medium) · #dp
- [ ] `interleaving-string` — Interleaving String (Medium) · #dp
- [ ] `edit-distance` — Edit Distance (Medium) · #dp
- [ ] `distinct-subsequences` — Distinct Subsequences (Hard) · #dp

### 15. Greedy
- [ ] `maximum-subarray` — Maximum Subarray (Medium) · #greedy #kadane
- [ ] `jump-game` — Jump Game (Medium) · #greedy
- [ ] `jump-game-ii` — Jump Game II (Medium) · #greedy
- [ ] `gas-station` — Gas Station (Medium) · #greedy
- [ ] `hand-of-straights` — Hand of Straights (Medium) · #greedy #hashmap
- [ ] `merge-triplets-to-form-target-triplet` — Merge Triplets (Medium) · #greedy
- [ ] `partition-labels` — Partition Labels (Medium) · #greedy #two-pointers
- [ ] `valid-parenthesis-string` — Valid Parenthesis String (Medium) · #greedy

### 16. Intervals
- [ ] `insert-interval` — Insert Interval (Medium) · #intervals
- [ ] `merge-intervals` — Merge Intervals (Medium) · #intervals #sort
- [ ] `non-overlapping-intervals` — Non-overlapping Intervals (Medium) · #intervals #greedy
- [ ] `meeting-rooms` — Meeting Rooms (Easy) · #intervals
- [ ] `meeting-rooms-ii` — Meeting Rooms II (Medium) · #intervals #heap
- [ ] `minimum-interval-to-include-each-query` — Min Interval to Include Query (Hard) · #intervals #heap

### 17. Math & Geometry
- [ ] `rotate-image` — Rotate Image (Medium) · #matrix
- [ ] `spiral-matrix` — Spiral Matrix (Medium) · #matrix
- [ ] `set-matrix-zeroes` — Set Matrix Zeroes (Medium) · #matrix
- [ ] `happy-number` — Happy Number (Easy) · #math #fast-slow
- [ ] `plus-one` — Plus One (Easy) · #math
- [ ] `pow-x-n` — Pow(x, n) (Medium) · #math
- [ ] `multiply-strings` — Multiply Strings (Medium) · #math

### 18. Bit Manipulation
- [ ] `single-number` — Single Number (Easy) · #bit #xor
- [ ] `number-of-1-bits` — Number of 1 Bits (Easy) · #bit
- [ ] `counting-bits` — Counting Bits (Easy) · #bit #dp
- [ ] `reverse-bits` — Reverse Bits (Easy) · #bit
- [ ] `missing-number` — Missing Number (Easy) · #bit #xor
- [ ] `sum-of-two-integers` — Sum of Two Integers (Medium) · #bit
- [ ] `reverse-integer` — Reverse Integer (Medium) · #bit #math

---

## Слабые паттерны (заполняется по ходу)

Сюда скилл/пользователь заносит паттерны, где было тяжело (по пометкам в `notes.md`). Для точечного возврата и повторного прорешивания.

- **defaultdict + группировка по каноническому ключу** (`group-anagrams`, 2026-07-11): путаница `d[key] = v` vs `d[key].append(v)`; забыл `list(d.values())` на возврате. Вернуться на похожей задаче с группировкой.
- **bucket-sort (частота как индекс)** (`top-k-frequent-elements`, 2026-07-12): размер корзин `len(nums)+1`, не `len(nums)` (off-by-one → IndexError); корзина обязана быть списком (коллизия по частоте); не переиспользовать имя параметра как переменную цикла. Вернуться на задаче, где ключ кодируется индексом массива.
