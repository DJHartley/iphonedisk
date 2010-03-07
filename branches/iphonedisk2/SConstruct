env = Environment(CCFLAGS = '-I.')
Export('env')

proto = SConscript('proto/SConscript')
Export('proto')

rpc = SConscript('rpc/SConscript')
Export('rpc')

fs = SConscript('fs/SConscript')
Export('fs')
