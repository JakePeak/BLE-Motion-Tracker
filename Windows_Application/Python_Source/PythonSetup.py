import sys, os
path = os.getcwd()
if path not in sys.path:
    sys.path.append(path)

print(sys.path)
