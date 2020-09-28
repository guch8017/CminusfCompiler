import os
INPUT_FILE_DIR = './tests/lab1/testcase'
OUTPUT_FILE_DIR = './tests/lab1/token'
TA_TOKEN_FILE_DIR = './tests/lab1/TA_token'

all_files = os.listdir(INPUT_FILE_DIR)
target_files = []
for f in all_files:
    if f.endswith(".cminus"):
        target_files.append(f)
print("Find %d files" % len(target_files))

if not os.path.exists(OUTPUT_FILE_DIR):
    os.makedirs(OUTPUT_FILE_DIR)

for f in target_files:
    print(f"TEST CASE {f} :", end='')
    input_file_path = os.path.join(INPUT_FILE_DIR, f)
    output_file = f.split('.cminus')[0] + '.tokens'
    diff_file = output_file.replace('tokens', 'diff')
    output_file_path = os.path.join(OUTPUT_FILE_DIR, output_file)
    ta_token_path = os.path.join(TA_TOKEN_FILE_DIR, output_file)
    os.popen('./build/lexer %s %s' % (input_file_path, output_file_path)).read()
    diff = os.popen(f'diff {output_file_path} {ta_token_path}').read()
    if diff:
        print(f"\033[31m\tFAILED!\033[0m\n{diff}")
    else:
        print("\033[32m\tSUCCESS!\033[0m")

    