import fnmatch
import os
import sys

matches = []
for root, dirnames, files in os.walk('src'):
  for f in fnmatch.filter(files, '*.cc') + fnmatch.filter(files, '*.h'):
    matches.append(os.path.join(root, f))

path_prefix = os.path.join(os.path.dirname(__file__), '..', 'build', 'bin')
if sys.platform.startswith('win'):
  binary = os.path.join(path_prefix, 'win', 'clang-format.exe')
else:
  binary = os.path.join(path_prefix, 'mac', 'clang-format')
for match in matches:
  args = [binary, '-i', match]
  os.system(' '.join(args))
