"""
Contains Duplicate — leetcode.com/problems/contains-duplicate/
Паттерн: hashset (виденные элементы)
Время: O(n)   Память: O(n)
"""
from typing import List


class Solution:
    def containsDuplicate(self, nums: List[int]) -> bool:
        elements = set()
        for idx, el in enumerate(nums):
            if el in elements:
                return True
            elements.add(el)
        return False
