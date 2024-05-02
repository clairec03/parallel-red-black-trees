import random
import os

max_int, min_int = 2 ** 31 - 1, -2 ** 31
operations = {
    "insert" : ["INSERT"],
    "both" : ["INSERT", "DELETE"]
}

def insert(f, op, num_ops, val_pool=None):
    f.write(f"{op} {num_ops}\n")      
    str_values = [str(random.randint(min_int, max_int)) for _ in range(num_ops)]
    if val_pool is not None: val_pool.update(str_values)
    f.write(" ".join(str_values) + "\n")

def generate_test_cases(fpath, num_ops=10, max_len=1000, optype="both"):
    with open(fpath, "w") as f:
        if optype == "insert":
            for _ in range(num_ops):
                op = random.choice(operations[optype])
                num_ops = random.randint(1, max_len)
                insert(f, op, num_ops, val_pool=None)
        elif optype == "both":
            val_pool = set()
            for _ in range(num_ops):
                op = random.choice(operations["both"])
                num_ops = random.randint(1, max_len)          
                if op == "INSERT":
                    insert(f, op, num_ops, val_pool)
                elif op == "DELETE":
                    prev_len = len(val_pool)                    
                    if prev_len == 0:
                        insert(f, "INSERT", num_ops, val_pool=None)
                        continue
                    str_values = random.sample(list(val_pool), min(num_ops, prev_len))
                    val_pool.difference_update(str_values)
                    f.write(f"{op} {len(str_values)}\n")      
                    f.write(" ".join(str_values) + "\n")
                    
filename = input("Enter the name of the file to write the test cases to: ")
num_ops = input("Enter the number of operations to generate: ")
if len(num_ops) == 0:
    print("No number of operations specified. Defaulting to 10.")
    num_ops = 10
max_len = input("Enter the maximum number of values in each operation: ")
if len(max_len) == 0:
    print("No maximum length specified. Defaulting to 1000.")
    max_len = 1000
if int(max_len) < 1:
    print("Invalid maximum length. Defaulting to 1000.")
    max_len = 1000

user_op_input = input("Enter the operation type (insert, both): ").strip().lower()

if user_op_input not in operations:
    print("Invalid operation type. Defaulting to both.")
    user_op_input = "both"

filepath = os.getcwd() + f"/inputs/{filename}"
generate_test_cases(filepath, int(num_ops), int(max_len), user_op_input)