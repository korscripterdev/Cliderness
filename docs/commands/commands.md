# Cliderness command reference

Generated with `cliderness --list-commands` (version 0.1.0). Created by korscripterdev.

Every command below has a real handler and is covered by `tests/test_commands.c`. Cheat commands need `/cheats on` (or *Allow cheats* when creating the world). `/help <command>` shows the localised description in game.

| command | category | permission | syntax |
|---|---|---|---|
| `/gamemode` | gamemode | cheat | `<survival\|creative\|adventure\|spectator>` |
| `/god` | player | cheat | `[on\|off]` |
| `/noclip` | player | cheat | `[on\|off]` |
| `/fly` | player | cheat | `[on\|off]` |
| `/heal` | player | cheat | `[amount]` |
| `/feed` | player | cheat |  |
| `/hydrate` | player | cheat |  |
| `/rest` | player | cheat |  |
| `/stamina` | player | cheat | `<set\|get> [0-100]` |
| `/health` | player | cheat | `<set\|get> [hp]` |
| `/hunger` | player | cheat | `<set\|get> [0-100]` |
| `/thirst` | player | cheat | `<set\|get> [0-100]` |
| `/fatigue` | player | cheat | `<set\|get> [0-100]` |
| `/temperature` | player | cheat | `<set\|get> [30-42]` |
| `/attribute` | player | cheat | `<set\|get> <stat> [1-100]` |
| `/skill` | player | cheat | `<set\|get> <skill> [0-100]` |
| `/kill` | player | cheat | `[@s\|@e\|@e[type=item]]` |
| `/respawn` | player | any |  |
| `/spawnpoint` | position | cheat |  |
| `/crouch` | movement | any |  |
| `/tp` | position | cheat | `<x> <y> <z> \| spawn` |
| `/pos` | position | any |  |
| `/top` | position | cheat |  |
| `/up` | position | cheat | `<blocks>` |
| `/spawn` | position | cheat |  |
| `/stats` | player | any |  |
| `/time` | time | any | `<set\|add\|query> [noon\|midnight\|HH:MM\|N[s\|m\|h\|d]]` |
| `/timescale` | time | cheat | `[multiplier]` |
| `/daylength` | time | cheat | `[real seconds per game day]` |
| `/weather` | weather | any | `<set\|clear\|query\|list> [type] [minutes]` |
| `/season` | time | any | `<set\|query> [spring\|summer\|autumn\|winter]` |
| `/seed` | world | any |  |
| `/biome` | world | any |  |
| `/biomes` | world | any |  |
| `/locate` | world | cheat | `<biome>` |
| `/save` | system | any |  |
| `/world` | world | any |  |
| `/setblock` | block | cheat | `<x> <y> <z> <block>` |
| `/fill` | building | cheat | `<x1> <y1> <z1> <x2> <y2> <z2> <block>` |
| `/replace` | building | cheat | `<x1> <y1> <z1> <x2> <y2> <z2> <from> <to>` |
| `/clone` | building | cheat | `<x1> <y1> <z1> <x2> <y2> <z2> <x> <y> <z>` |
| `/undo` | building | cheat |  |
| `/redo` | building | cheat |  |
| `/flatten` | terrain | cheat | `[radius]` |
| `/raise` | terrain | cheat | `[radius] [height]` |
| `/lower` | terrain | cheat | `[radius] [depth]` |
| `/smooth` | terrain | cheat | `[radius]` |
| `/drain` | fluid | cheat | `[radius]` |
| `/flood` | fluid | cheat | `[radius]` |
| `/blockinfo` | block | any | `[x y z]` |
| `/give` | item | cheat | `[@s] <item> [count]` |
| `/clear` | item | cheat | `[@s] [item]` |
| `/inventory` | item | any | `<list\|clear\|sort>` |
| `/hotbar` | item | any | `<1-9>` |
| `/iteminfo` | item | any | `[item]` |
| `/items` | item | any | `[filter]` |
| `/blocks` | block | any | `[filter]` |
| `/durability` | durability | cheat | `<set\|get> [1-100 %]` |
| `/repair` | durability | cheat |  |
| `/help` | system | any | `[command\|page]` |
| `/commands` | system | any |  |
| `/version` | system | any |  |
| `/credits` | system | any |  |
| `/cheats` | settings | any | `<on\|off>` |
| `/language` | settings | any | `<code\|list>` |
| `/mem` | debug | any |  |
| `/chunks` | debug | any |  |
| `/entities` | debug | any |  |
| `/clearlog` | system | any |  |
| `/quit` | system | any |  |
| `/debug` | debug | any | `[on\|off]` |

total top-level commands: 71 (aliases: 22)
