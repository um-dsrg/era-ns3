from pathlib import Path

def Settings(**kwargs):
    homePath = str(Path.home())
    return {
        'flags': ['-x', 'c++', '-Wall', '-Wextra', '-Werror', '-std=c++14',
                  '-I',
                  homePath + '/Documents/Git/Development/Ns3/build/debug',
                  '-I',
                  '/usr/local/include'],
    }
