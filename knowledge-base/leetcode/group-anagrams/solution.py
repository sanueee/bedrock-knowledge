"""
Group Anagrams — leetcode.com/problems/group-anagrams/
Паттерн: hashmap с каноническим ключом (defaultdict(list))
Время: O(n·k log k)   Память: O(n·k)   (n строк, k — длина строки)
"""
from typing import List
from collections import defaultdict


class Solution:
    def groupAnagrams(self, strs: List[str]) -> List[List[str]]:
        d = defaultdict(list)
        for s in strs:
            key = "".join(sorted(s))
            d[key].append(s)
        return list(d.values())
