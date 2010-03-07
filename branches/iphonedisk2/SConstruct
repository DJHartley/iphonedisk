env = Environment(CCFLAGS = '-Werror -Wall -I.')
Export('env')

proto = SConscript('proto/SConscript')
Export('proto')

[ rpc_channel, rpc_service ] = SConscript('rpc/SConscript')
Export('rpc_channel', 'rpc_service')

fs = SConscript('fs/SConscript')
Export('fs')

SConscript('test/SConscript')
