"""
Top K Frequent Elements — leetcode.com/problems/top-k-frequent-elements/
Паттерн: hashmap + bucket-sort
Время: O(n)   Память: O(n)
"""
from typing import List
from collections import Counter

class Solution:
    def topKFrequent(self, nums: List[int], k: int) -> List[int]:
        arr = [[] for _ in range(len(nums) + 1)]
        c = Counter(nums) 
        for key, v in c.items():
            arr[v].append(key)
        counter = 0
        pos = len(nums) - 1
        res = []
        while counter < k:
            if arr[pos]:
                for x in arr[pos]:
                    if counter < k:
                        res.append(x)
                        counter += 1
            pos -= 1
        return res

