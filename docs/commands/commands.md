# Cliderness command reference

Generated with `cliderness --list-commands`. Created by korscripterdev.

Every command below has a real handler and is covered by the test suite. Cheat commands need `/cheats on` (or *Allow cheats* when creating the world). `/help <command>` shows the localised description in game.

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
| `/kill` | player | cheat | `[@s\|@e\|@e[type=item\|mob\|<creature>]]` |
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
| `/craft` | item | any | `<recipe> [count]` |
| `/recipes` | item | any | `[available\|filter]` |
| `/recipe` | item | any | `<recipe>` |
| `/stations` | item | any |  |
| `/fuel` | item | any |  |
| `/effect` | player | any | `<list\|give\|clear> [effect] [minutes]` |
| `/cure` | player | cheat |  |
| `/sleep` | player | any |  |
| `/skipnight` | time | cheat |  |
| `/grow` | world | cheat | `[radius]` |
| `/crops` | world | any |  |
| `/equip` | equipment | any | `<item>` |
| `/unequip` | equipment | any | `<head\|body\|legs\|feet\|back\|all>` |
| `/equipment` | equipment | any |  |
| `/chests` | item | any |  |
| `/fluid` | fluid | any | `[on\|off]` |
| `/water` | fluid | cheat | `[level 1-15]` |
| `/lava` | fluid | cheat | `[level 1-15]` |
| `/settle` | fluid | cheat | `[steps]` |
| `/volume` | fluid | any | `[radius]` |
| `/structures` | world | any |  |
| `/findstructure` | world | cheat | `<structure>` |
| `/summon` | creatures | cheat | `<creature> [count]` |
| `/creatures` | creatures | any | `[animal\|monster\|npc]` |
| `/mobs` | creatures | any |  |
| `/difficulty` | world | any | `[peaceful\|easy\|normal\|hard]` |
| `/spawning` | creatures | cheat | `[on\|off]` |
| `/despawn` | creatures | cheat | `[creature\|all]` |
| `/bestiary` | creatures | any | `<creature>` |
| `/attack` | combat | any |  |
| `/trade` | creatures | any |  |
| `/threat` | combat | any |  |
| `/kills` | combat | any |  |
| `/ships` | ships | any |  |
| `/boats` | ships | any |  |
| `/board` | ships | any |  |
| `/disembark` | ships | any |  |
| `/throttle` | ships | any | `<-1..3>` |
| `/anchor` | ships | any | `[on\|off]` |
| `/shipinfo` | ships | any |  |
| `/bail` | ships | any |  |
| `/hull` | ships | cheat | `set <0-100>` |
| `/spawnship` | ships | cheat | `<ship>` |
| `/sinkship` | ships | cheat |  |
| `/wind` | weather | any | `[direction\|0-255]` |
| `/dimension` | world | any |  |
| `/dimensions` | world | any |  |
| `/travel` | world | cheat | `<overworld\|ancient> [portal]` |
| `/makeportal` | world | cheat |  |
| `/ancientbiomes` | world | any |  |

total top-level commands: 121 (aliases: 37)
