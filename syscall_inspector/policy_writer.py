#!/usr/bin/python3 -EOO

# -------------------------
# Furnace (c) 2017-2018 Micah Bushouse
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# -------------------------
"""
Command line interface to create syscall inspector source code.
"""

import sys
import os
import argparse
import pprint

SEVEN_SPACE = '                            '
SIX_SPACE = '                        '
FIVE_SPACE = '                    '

def do_trace_processing(tokens):
    """
    Example: A TRACE connect RDI_INT
    Output: {'socket': {'RDI': 'INT'}}
    """
    s = tokens[2]

    try:
        reg, casttype = tokens[3].split('_')
        out = {s: {reg: casttype}}
    except IndexError:
        out = {s: {}}

    return out


def do_arg_processing(look_tokens):
    """
    Example 1: RDI_PTR:dirname|/usr/lib64/python3.6/lib-dynload,
    Output 1: {'reg': 'RDI', 'type': 'PTR', 'trans': 'dirname', 'arg': '/usr/lib64/python3.6/lib-dynload'}
    Example 2: RSI_INT:module_termios.TCGETS,
    Output 2: {'reg': 'RDI', 'type': 'INT', 'arg': 123}
    Example 3: RSI_INT:21524
    Output 3: {'reg': 'RSI', 'type': 'INT', 'arg': 21524}
    """
    l = look_tokens[0].replace(',', '')
    regpair, argpair = l.split(':')
    reg, casttype = regpair.split('_')

    if '|' in argpair:
        trans, arg = argpair.split('|')
        return {'reg': reg, 'type': casttype, 'trans': trans, 'arg': arg}

    elif 'module' in argpair:
        modpair, const_name = argpair.split('.')
        _, mod = modpair.split('_')
        loaded_mod = __import__(mod)
        const_value = int(getattr(loaded_mod, const_name))
        return {'reg': reg, 'type': casttype, 'arg': const_value}

    else:
        return {'reg': reg, 'type': casttype, 'arg': int(argpair)}


def parse_policy(fname):
    with open(fname, 'r') as fh:
        raw = fh.read()
        print(f'read {len(raw)}B policy from {fname}')

    allow_list = []
    traced_list = []

    test_dict = {}
    init_list = []
    postinit_list = []
    policy_map_list = []

    state = ''

    lines = raw.split('\n')
    totallines = len(lines)
    curline = 0

    while curline < totallines:
        l = lines[curline]
        tokens = l.split()

        # empty line
        if not len(tokens):
            curline += 1
            print(f'{curline} Empty line')
            continue

        # comment
        if tokens[0].startswith('#'):
            curline += 1
            print(f'{curline} Comment')
            continue

        # SECCOMP will always pass these, the syscall inspector will never see them.
        elif tokens[0] == 'ALLOW':
            allow_list.append(tokens[1])
            print(f'{curline} ALLOW_LIST: +{tokens[1]}')

        # Trace the syscall and always perform this action regardless of policy level.
        elif tokens[0] == 'A':

            if tokens[1] == 'TRACE':
                out = do_trace_processing(tokens)
                traced_list.append(out)
                print(f'{curline} TRACED_LIST: +{out}')

            elif tokens[1] == 'TEST_ARGS':
                call = tokens[2]
                print(f'{curline} TEST_ARGS for {call}')
                lookaheadline = curline + 1
                look_l = lines[lookaheadline]
                arglist = []

                while look_l != '}':
                    look_tokens = look_l.split()

                    argdict = do_arg_processing(look_tokens)
                    arglist.append(argdict)
                    print(f'{lookaheadline} {look_l} {argdict}')

                    lookaheadline += 1
                    look_l = lines[lookaheadline]

                print(f'{lookaheadline} TEST_ARGS done: +{out}')
                test_dict[call] = arglist
                curline = lookaheadline

        elif tokens[2] == 'open' and tokens[0] == '1':

            call = tokens[2]
            print(f'{curline} OPEN start')
            lookaheadline = curline + 1
            look_l = lines[lookaheadline]
            argdict = {'dirname': [], 'dirname_glob': [], 'basename': [],
                       'exactpath': []}

            while look_l.strip() != '}':
                try:
                    trans, path = look_l.split(':')[1].split('|')
                except IndexError:
                    pass
                finally:
                    path = path.replace(',', '')
                    argdict[trans].append(path)
                    print(f'{lookaheadline} {path}')

                lookaheadline += 1
                look_l = lines[lookaheadline]

            print(f'{lookaheadline} OPEN done: +{out}')
            open_dict = argdict
            print(f'{open_dict}')
            curline = lookaheadline

        elif tokens[0] == '1':
            if tokens[1] == 'TRACE':
                call = tokens[2]
                policy_map_list.append(call)
                print(f'{curline} 1 TRACE +{call}')

        # TODO would require some work...
        # Trace the syscall and perform this action at policy level 0.
        #elif tokens[0] == '0':
            #if tokens[1] == 'TRACE':
                #reg, casttype = tokens[3].split('_')
            #print(f'{curline} 0: +{tokens[1]}')

        # TODO
        # Trace the syscall and perform this action at policy level 1.
        #elif tokens[0] == '1':
            #print(f'{curline} 1: +{tokens[1]}')

        else:
            print(f'{curline} Unknown line')

        curline += 1


    return {'allow_list': allow_list,
            'test_dict': test_dict,
            'open_dict': open_dict,
            'init_list': init_list,
            'postinit_list': postinit_list,
            'policy_map_list': policy_map_list,
            'traced_list': traced_list}


def main():
    """
    Parses command line input, parses the policy, then writes C source to output file.
    """

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--policy",
        dest="policy",
        default="app.policy",
        metavar="policy",
        help="Policy file",
    )
    parser.add_argument(
        "--master",
        dest="master",
        default="master_inspector.c",
        metavar="master",
        help="Master source file",
    )
    parser.add_argument(
        "--output",
        dest="output",
        default="app_inspector.c",
        metavar="output",
        help="Output source file",
    )
    args = parser.parse_args()
    print(args)


    master = args.master
    gen = args.output

    pol = parse_policy(args.policy)

    source = ''
    with open(master, 'r') as fh:
        source = fh.read()
        print(f'read {len(source)}B from {master}')

    if not source:
        print('source was empty')
        sys.exit()

    # ALLOW_LIST
    print('parsing allow_list')
    pprint.pprint(pol['allow_list'])
    allow_out = ''
    for s in pol['allow_list']:
        allow_out += f'    rc |= seccomp_rule_add(*ctx, SCMP_ACT_ALLOW, SCMP_SYS({s}), 0);\n'

    # TRACED_LIST
    print('parsing traced_list')
    pprint.pprint(pol['traced_list'])
    rule_add_out = ''
    switch_out = ''
    for s in pol['traced_list']:
        call = [*s][0]

        if call in pol['test_dict']:
            print(f'skipping {call} since it is in test_dict')
            continue

        rule_add_out += f'    rc |= seccomp_rule_add(*ctx, SCMP_ACT_TRACE(2), SCMP_SYS({call}), 0);\n'

        # We only need to include cases in the switch statement that will
        # be treated differently than the default.  The default prints
        # the syscall but none of its arguments.
        try:
            reg = [*s[call]][0]
            cast_type = s[call][reg]

            func_type = ''
            if cast_type == 'PTR':
                func_line = f'read_reg_string({reg}, pid, orig_file);'
                print_line = f'PRINT_DEBUG("[{call} %s]\\n", orig_file);'
            elif cast_type == 'INT':
                func_line = ''
                print_line = f'PRINT_DEBUG("[{call} %d]\\n", read_reg_int({reg}, pid));'

            switch_out += f'''                    case __NR_{call}:
                        {func_line}
                        {print_line}
                        break;\n'''
        except IndexError:
            continue

    # TEST_DICT
    print('parsing test_dict')
    pprint.pprint(pol['test_dict'])
    test_out = ''
    for call, arglist in pol['test_dict'].items():

        argdict = arglist.pop()
        reg = argdict['reg']
        cast_type = argdict['type']

        func_type = ''
        if cast_type == 'PTR':
            func_line = f'read_reg_string({reg}, pid, orig_file);'
            print_line = f'PRINT_DEBUG("[{call} %s]\\n", orig_file);'
            switch_line = 'orig_file'
            #TODO
        elif cast_type == 'INT':
            func_line = ''
            print_line = f'PRINT_DEBUG("[{call} %d]\\n", read_reg_int({reg}, pid));'
            switch_line = 'pid'
            const_value = argdict['arg']
            switch_body = f'case {const_value}: break;\n'

        for argdict in arglist:
            const_value = argdict['arg']
            switch_body += f'{SEVEN_SPACE}case {const_value}: break;\n'

        test_out += f'''{FIVE_SPACE}case __NR_{call}:
                        {func_line}
                        {print_line}
                        switch ({switch_line})
                        {{
                            {switch_body}
                            default:
                                PRINT_DEBUG("{call} {reg} disallowed!\\n");
                                return 0;
                        }}
                        break;\n'''

    # open_dict
    print('parsing open_dict')
    pprint.pprint(pol['open_dict'])
    open_out = ''
    for item in pol['open_dict']['dirname_glob']:
        open_out += f'{SEVEN_SPACE}if (strncmp("{item}", path, strlen("{item}")) == 0) break;\n'
    for item in pol['open_dict']['dirname']:
        open_out += f'{SEVEN_SPACE}if (strcmp("{item}", path) == 0) break;\n'
    for item in pol['open_dict']['basename']:
        open_out += f'{SEVEN_SPACE}if (strcmp("{item}", base) == 0) break;\n'
    for item in pol['open_dict']['exactpath']:
        dirname = os.path.dirname(item)
        basename = os.path.basename(item)
        open_out += f'{SEVEN_SPACE}if ((strcmp("{dirname}", path) == 0) && (strcmp("{basename}", base) == 0)) break;\n'

    # POLICY_MAP_LIST
    print('parsing policy_map_list')
    pprint.pprint(pol['policy_map_list'])
    policy_map_out = ''
    for item in pol['policy_map_list']:
        policy_map_out += f'{SEVEN_SPACE}policy_map[__NR_{item}] = 1;\n'


    source = source.replace('//ALLOW_LIST', allow_out)
    print(f'added {len(allow_out)}B for ALLOW_LIST')
    source = source.replace('//TRACED_LIST_SWITCH', switch_out)
    print(f'added {len(switch_out)}B for TRACED_LIST switch_out')
    source = source.replace('//TRACED_LIST', rule_add_out)
    print(f'added {len(rule_add_out)}B for TRACED_LIST add_rule')
    source = source.replace('//TEST_LIST', test_out)
    print(f'added {len(test_out)}B for TEST_LIST test_out')
    source = source.replace('//OPEN_LIST', open_out)
    print(f'added {len(open_out)}B for open_dict open_out')
    source = source.replace('//POLICY_MAP_LIST', policy_map_out)
    print(f'added {len(policy_map_out)}B for POLICY_MAP_LIST policy_map_out')







    # Print output
    print(f'writing {len(source)}B to {gen}')
    with open(gen, 'w') as fh:
        fh.write(source)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("caught keyboard interrupt")
