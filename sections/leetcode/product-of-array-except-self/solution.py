"""
Product of Array Except Self — leetcode.com/problems/product-of-array-except-self/
Паттерн: prefix / suffix products
Время: O(n)   Память: O(1) доп. (выходной массив не считаем)
"""
from typing import List


class Solution:
    def productExceptSelf(self, nums: List[int]) -> List[int]:
        l = len(nums)
        res = [0] * l
        p = 1
        for i in range(l):
            res[i] = p
            p *= nums[i]

        p = 1
        for i in range(l - 1, -1, -1):
            res[i] *= p
            p *= nums[i]
        
        return res

if __name__ == "__main__":
    print(Solution().productExceptSelf([1, 2, 3, 4]))