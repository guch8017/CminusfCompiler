import sys
import os

if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] != "False":
        print("[Judger] Running in easy mode. Output of diff will not show on screen")
        easy = True
    else:
        print("[Judger] Running in complex mode")
        easy = False
    if 'build' not in os.listdir():
        print("[Judger] build dir not exist. mkdir(build)...")
        os.mkdir('build')
    print("[Judger] Entering " + os.getcwd() + "/build/")
    os.chdir('build')
    print("[Judger] Calling cmake")
    status = os.system("cmake ..")
    if status != 0:
        print("[Judger] Camke error. Please check cmake log for detail")
        exit(0)
    print("[Judger] Calling make clean")
    os.system("make clean")
    print("[Judger] Calling make")
    status = os.system("make")
    if status != 0:
        print("[Judger] Make error. Please check make log for detail")
        exit(0)
    print("[Judger] Leaving build folder")
    os.chdir("..")
    print("[Judger] Build finished successfully!")
    if not os.path.exists("tests/lab2/syntree_easy_generate"):
        os.mkdir("tests/lab2/syntree_easy_generate")
    if not os.path.exists("tests/lab2/syntree_normal_generate"):
        os.mkdir("tests/lab2/syntree_normal_generate")
    for case in ['normal', 'easy']:
        print(f"[Judger] Judging {case} cases...")
        for filename in os.listdir(f"tests/lab2/{case}/"):
            if not filename.endswith(".cminus"):
                continue
            print(f"[Judger] Juding tests/lab2/{case}/{filename}", end='')
            os.popen(f"build/parser < tests/lab2/{case}/{filename} > tests/lab2/syntree_{case}_generate/{filename[:-6]}syntax_tree").read()
            diff = os.popen(f'diff tests/lab2/syntree_{case}_generate/{filename[:-6]}syntax_tree tests/lab2/syntree_{case}_std/{filename[:-6]}syntax_tree').read()
            if diff:
                print("\t \033[31m\Failed\033[0m")
                if easy:
                    print(diff)
            else:
                print("\t \033[32mPassed\033[0m")
