"""
Two Sum — leetcode.com/problems/two-sum/
Паттерн: hashmap (число → индекс)
Время: O(n)   Память: O(n)
"""
from typing import List


class Solution:
    def twoSum(self, nums: List[int], target: int) -> List[int]:
        pairs = dict()
        list_len = len(nums)
        for i in range(list_len):
            need = target - nums[i]
            if need in pairs:
                return pairs[need], i
            pairs[nums[i]] = i
