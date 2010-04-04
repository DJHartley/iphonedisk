env = Environment(CCFLAGS = '-Werror -Wall -I. ')
Export('env')

mode = ARGUMENTS.get('mode', 'release')
if not (mode in ['debug', 'release']):
  print "Error: expected 'debug' or 'release', found: " + mymode
  Exit(1)

if mode == 'debug':
  env.Append(CCFLAGS = '-g -DDEBUG ')

proto = SConscript('proto/SConscript')
Export('proto')

[ rpc, rpc_channel, rpc_service ] = SConscript('rpc/SConscript')
Export('rpc', 'rpc_channel', 'rpc_service')

fs = SConscript('fs/SConscript')
Export('fs')

mount = SConscript('mount/SConscript')
Export('mount')

SConscript('mobilefs/SConscript')
SConscript('test/SConscript')
