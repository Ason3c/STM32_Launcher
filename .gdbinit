define flash
file main.elf
load
end

define reconnect
target extended-remote localhost:4242
file main.elf
set var {int}0x40021024=0x01000000
break shutdown
end

source -v armv7m-macros.gdb
reconnect