env = Environment(CCFLAGS = '-Werror -Wall -I. ')
Export('env')

arch = ARGUMENTS.get('arch')
if arch:
  env.Append(CCFLAGS = '-arch %s ' % arch)
  env.Append(LINKFLAGS = '-arch %s ' % arch)

mode = ARGUMENTS.get('mode', 'release')
if not (mode in ['debug', 'release']):
  print "Error: expected 'debug' or 'release', found: " + mode
  Exit(1)

if mode == 'debug':
  env.Append(CCFLAGS = '-g -DDEBUG ')

proto = SConscript('proto/SConscript')
Export('proto')

rpc = SConscript('rpc/SConscript')
Export('rpc')

fs = SConscript('fs/SConscript')
Export('fs')

mount = SConscript('mount/SConscript')
Export('mount')

SConscript('mobilefs/SConscript')
SConscript('test/SConscript')
