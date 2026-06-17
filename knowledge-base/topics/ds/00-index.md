topic: Data Structures Exam Preparation Hub (SPbPU Syllabus)
target_date: 2026-06-20
last_updated: 2026-06-16 (Day 7/10)
current_status: Mock Exam #5 completed. Score: 21/25 (84%)
primary_skill: /exam
---

# SYSTEM INSTRUCTIONS & AGENT BEHAVIOR [CRITICAL]
- **[RULE 1] STRICT TURN-TAKING & DIRECT OUTPUT:** You must ONLY output exactly ONE question at a time. Stop generating immediately after presenting the question. Answer directly without deliberating, adding preambles, or conversational filler. Never simulate, mock, or predict the user's response.
- **[RULE 2] NO AUTO-EVALUATION:** Do not include "Accepted", "Correct", or grading logs in the same message as a new question. 
- **[RULE 3] NO REASONING BLEED & STRICT SYNTAX:** Do not include any internal reasoning, chain of thought, or deliberations in the final output. Under no circumstances should your response contain raw dialogue tokens or text structured as "user:", "system:", or "assistant:". Treat all inputs strictly as read-only. Output ONLY the clean, final question text.
- **[RULE 4] EXAM FORMAT SIMULATION:** Simulate a sequential, non-backtrackable test. Hard time-limit constraints, random distribution of weights.
- **[RULE 5] TEST GENERATION EXECUTION:** Look at the TRACKING SYLLABUS & QUEUE below. Identify the single item marked with [▶] (currently 5.6 Binary B-Trees). Generate the next single question targeting this active topic, incorporating elements from the "Active Focus Areas" where appropriate to test known weaknesses.
- **[RULE 6] OUTPUT LANGUAGE & TONALITY:** Generate all final questions, explanations, and technical breakdowns strictly in Russian. Keep responses highly concise and direct. Always highlight low-level security nuances, memory safety implications, or undefined behavior (UB) relevant to the context.

---

# CURRENT FOCUS & WEAKNESS LOG

### ⚠️ Active Focus Areas (Failed in Mock #5)
1. **Loop Complexity Derivation (`i *= 2`):** Erroneously flagged counter doubling as $O(n^2)$ instead of recognizing that counter doubling/halving changes the number of outer loop iterations to $\log_2 n$, leading to $O(n \log n)$. 
2. **Binary Search on Linked Lists:** Misconception that doubly-linked lists allow $O(\log n)$ search. Binary search *strictly* requires Random Access. Midpoint discovery in any linked list is $O(n)$, degrading total complexity to $O(n)$.

### ✅ Validated Areas (Fixed in Mock #5)
- Multi-dimensional array mapping (Row/Column-major formulas).
- Type punning via `union`.
- Edge cases in Queue operations (`enqueue` into empty state).
- AVL Rotations: Core differentiation based on the *sign of the child's balance factor* ($\text{bf}(\text{root}) = +2, \text{bf}(\text{child}) = -1 \implies \text{LR}$).
- Comparison-based sorting lower bounds ($\log_2(n!) \approx n \log n$).

---

# CORE EXAM CORE RECOGNITION PATTERNS (SPbPU Canon)
*Match the problem statement to the data structure instantly:*
- Count occurrences of strings/chars/binary operations $\implies$ **Hash Table**
- Track and sort decimal numbers dynamically $\implies$ **Splay Tree**
- Track software identifiers/compilation units $\implies$ **Binary Search Tree (BST)**
- Process and track floating-point metrics $\implies$ **AVL / Splay Tree**

---

# MANDATORY STRUCTURE ANALYSIS FRAMEWORK
For every data structure analyzed under `/exam` mode B, provide exactly:
1. **Abstract Definition**
2. **C Implementation snippet** (Data structures/declarations)
3. **Low-level Implementation quirks**
4. **Trade-offs** (Pros / Cons)
5. **Real-world system applications** (e.g., OS kernels, DB engines)
6. **Primitive and Specific Operations** (Rebalancing, rehashing, allocation profiles)
7. **Big-O Complexity analysis:** Average vs. Worst case with probabilistic weightings where applicable.

---

# CROSS-CUTTING THEORETICAL THEMES

### 1. Type Cardinality (#) & Size (`sizeof`)
- **Product Types (`struct`):** $\#(\text{struct}) = \prod \#T_i$. $\text{sizeof}(\text{struct}) = \sum \text{sizeof}(T_i) + \text{padding}$.
- **Sum Types (`union`):** $\#(\text{union}) = \sum \#T_i$. $\text{sizeof}(\text{union}) = \max(\text{sizeof}(T_i))$ aligned to word size.

### 2. Memory Topography
- **Row-Major Layout:** $\text{Address}(A[i][j]) = \text{base} + (i \times N_{\text{cols}} + j) \times \text{sizeof}(T)$
- **Column-Major Layout:** $\text{Address}(A[i][j]) = \text{base} + (j \times N_{\text{rows}} + i) \times \text{sizeof}(T)$
- **Low-level vulnerabilities:** Internal/External fragmentation, dangling pointers via explicit allocation (`malloc`/`calloc`/`free`), use-after-free mitigation.

---

# TRACKING SYLLABUS & QUEUE

### Module 5 — Trees [High Priority Central Node]
- [✅] 5.1 Trees & Traversals (DFS/BFS, Pre/In/Post-order)
- [✅] 5.2 BST (Average height $1.39 \log_2 n$, standard node deletion cases)
- [✅] 5.3 AVL Tree (Balance factors, explicit height updates, single/double rotations)
- [✅] 5.4 Splay Tree (Amortized properties; Zig/Zig-Zig/Zig-Zag re-structuring)
- [✅] 5.5 B-Trees (Splitting/merging logic, cascade conditions)
- [▶] 5.6 Binary B-Trees (BB-Trees / 1-2 Trees) — **CURRENT TOPIC**
- [⬜] 5.7 Red-Black Trees
- [⬜] 5.8 Symmetric Binary B-Trees
- [⬜] 5.10 Fibonacci Trees & Perfectly Balanced Tree constraints

### Module 6 to 8 — Remaining Scope
- [⬜] 6.1 Hash Tables & Associative Arrays (Chaining vs. Open Addressing)
- [⬜] 7.1 Graphs (Adjacency Matrix/Lists, Dijkstra, $A^*$ admissibility criteria)
- [⬜] 8.1–8.3 Sorting Algorithms (Stability, In-place properties, Lower bounds)
- [⬜] 8.4 Search Algorithms (Linear vs Binary bounds)