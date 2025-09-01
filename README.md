# Subarray Sum Counter

This program computes the total number of contiguous subarrays within a given array whose sum equals a specified target value `k`.

It processes input in JSON format and outputs the result as required by the test cases.

## Problem Statement

Given an array of integers `nums` and an integer `k`, return the total number of subarrays whose sum equals `k`.

A subarray is a contiguous non-empty sequence of elements within an array.

## Example

Input:

```json
{
  "keys": {
    "n": 4,
    "k": 3,
    "arr": [1, 2, 1, 2]
  }
}
```

Output:

```
2
```

Explanation: The subarrays `[1,2]` and `[2,1]` sum to `3`.

---

## Approach

* The solution leverages a prefix sum + hashmap technique.
* As we traverse the array, we maintain a running prefix sum.
* At each step, we check if `(prefix_sum - k)` has appeared before in the hashmap.

  * If yes, then there exists one or more subarrays ending at the current index with sum = `k`.
* The hashmap keeps track of how many times each prefix sum has occurred.
* This approach ensures **O(n)** time complexity with **O(n)** space.

---

## How to Run

1. Clone the repository or download the code file.
2. Ensure you have a C++ compiler installed (e.g., `g++`).
3. Compile the code:

   ```bash
   g++ -std=c++17 main.cpp -o subarray_sum
   ```
4. Run the program with input redirection:

   ```bash
   ./subarray_sum < input.json
   ```

   * Replace `input.json` with your test file in the required JSON format.

---

## Example Execution

input.json

```json
{
  "keys": {
    "n": 4,
    "k": 3,
    "arr": [1, 2, 1, 2]
  }
}
```

Command:

```bash
./subarray_sum < input.json
```

Output:

```
2
```

## Notes

* Handles large inputs efficiently.
* Uses `long long` to safely store prefix sums.
* Works for both positive and negative integers in the array.


Do you want me to also add a section explaining the structure of the JSON input (like what `n`, `k`, and `arr` represent), so the evaluator doesn’t have to infer it?
