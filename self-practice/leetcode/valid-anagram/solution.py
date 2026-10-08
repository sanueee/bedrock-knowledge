"""
Valid Anagram — leetcode.com/problems/valid-anagram/
Паттерн: hashmap / подсчёт частот (Counter)
Время: O(n + m)   Память: O(k) — число уникальных символов
"""
from collections import Counter


class Solution:
    def isAnagram(self, s: str, t: str) -> bool:
        c1 = Counter(s)
        c2 = Counter(t)
        return c1 == c2
