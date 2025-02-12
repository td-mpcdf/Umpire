##############################################################################
# Copyright (c) 2016-22, Lawrence Livermore National Security, LLC and Umpire
# project contributors. See the COPYRIGHT file for details.
#
# SPDX-License-Identifier: (MIT)
##############################################################################

formatters = {
    'RED': '\033[91m',
    'BLUE': '\033[94m',
    'END': '\033[0m',
}


errors = 0

def check_output(name, file_object, expected):
    global errors

    print("{BLUE}[RUN     ]{END} Checking for \"{expected}\" in {name}".format(name=name, expected=expected, **formatters))


    found = False

    for line in file_object:
        if expected in line.rstrip():
            found = True

    if not found:
        print("{RED}[   ERROR]{END} Got {contents}".format(contents=file_object.read(), expected=expected, **formatters))
        errors = errors + 1
    else:
        print("{BLUE}[      OK]{END} Found \"{expected}\" in {name}".format(name=name, expected=expected, **formatters))


def check_file_exists(filename):
    import os

    global errors

    print("{BLUE}[RUN     ]{END} Checking {myfile} exists".format(myfile=filename, **formatters))
    if not os.path.isfile(filename):
        print("{RED}[   ERROR]{END} {myfile} not found".format(myfile=filename, **formatters))
        errors += errors + 1
    else:
        print("{BLUE}[      OK]{END} {myfile} exists".format(myfile=filename, **formatters))

def check_file_not_exists(filename):
    import os

    global errors

    print("{BLUE}[RUN     ]{END} Checking {myfile} doesn't exist".format(myfile=filename, **formatters))
    if not os.path.isfile(filename):
        print("{BLUE}[      OK]{END} {myfile} doesn't exist".format(myfile=filename, **formatters))
    else:
        print("{RED}[   ERROR]{END} {myfile} found".format(myfile=filename, **formatters))
        errors += errors + 1


def run_io_test(test_env, file_uid, expect_logging, expect_replay):
    import subprocess
    import os

    global errors

    cmd_args = ['./io_tests']
    if expect_logging:
        cmd_args.append('--enable-logging')

    if expect_replay:
        cmd_args.append('--enable-replay')

    test_program = subprocess.Popen(cmd_args,
            env=dict(os.environ, **test_env),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            shell=False)
    pid = test_program.pid
    ecode = test_program.wait()

    output = test_program.stdout
    error = test_program.stderr

    if ecode != 0:
        print("{RED}[   ERROR]{END} Unexpected exit code {ecode} from io_test".format(ecode=ecode, **formatters))
        errors = 1
    else:
        check_output('stderr', error, b'testing error stream')

        output_filename = 'umpire_io_tests.{pid}.{uid}.log'.format(uid=file_uid, pid=pid)
        replay_filename = 'umpire_io_tests.{pid}.{uid}.replay'.format(uid=file_uid, pid=pid)

        if 'UMPIRE_OUTPUT_DIR' in test_env.keys():
            output_filename = '{dir}/umpire_io_tests.{pid}.{uid}.log'.format(dir=test_env['UMPIRE_OUTPUT_DIR'], uid=file_uid, pid=pid)
            replay_filename = '{dir}/umpire_io_tests.{pid}.{uid}.replay'.format(dir=test_env['UMPIRE_OUTPUT_DIR'], uid=file_uid, pid=pid)

        if expect_logging:
            check_file_exists(output_filename)
            with open(output_filename, 'rb') as output_file:
                check_output(output_filename, output_file, b'testing log stream')
        else:
            check_file_not_exists(output_filename)

        if expect_replay:
            check_file_exists(replay_filename)
            with open(replay_filename, 'rb') as replay_file:
                check_output(replay_filename, replay_file, b'testing replay stream')
        else:
            check_file_not_exists(replay_filename)


if __name__ == '__main__':
    import sys

    if sys.argv[1] == 'Basename +logging +replay' and errors == 0:
        print("{BLUE}[--------]{END} io_test 1/5 Basename +logging +replay Test".format(**formatters))
        run_io_test({'UMPIRE_OUTPUT_BASENAME' : 'umpire_io_tests'}, 0, True, True)

    if sys.argv[1] == 'Basename +Directory +logging +replay' and errors == 0:
        print("{BLUE}[--------]{END} io_test 2/5 Basename + Directory +logging +replay Test".format(**formatters))
        run_io_test({'UMPIRE_OUTPUT_DIR': './io_test_dir', 'UMPIRE_OUTPUT_BASENAME' : 'umpire_io_tests'}, 0, True, True)

    if sys.argv[1] == 'Basename +Directory -logging -replay' and errors == 0:
        print("{BLUE}[--------]{END} io_test 3/5 Basename + Directory -logging -replay Test".format(**formatters))
        run_io_test({'UMPIRE_OUTPUT_DIR': './optional_io_test_dir', 'UMPIRE_OUTPUT_BASENAME' : 'umpire_io_tests'}, 0, False, False)

    if sys.argv[1] == 'Basename +Directory +logging -replay' and errors == 0:
        print("{BLUE}[--------]{END} io_test 4/5 Basename + Directory +logging -replay Test".format(**formatters))
        run_io_test({'UMPIRE_OUTPUT_DIR': './optional_io_test_dir', 'UMPIRE_OUTPUT_BASENAME' : 'umpire_io_tests'}, 0, True, False)

    if sys.argv[1] == 'Basename +Directory -logging +replay' and errors == 0:
        print("{BLUE}[--------]{END} io_test 5/5 Basename + Directory -logging +replay Test".format(**formatters))
        run_io_test({'UMPIRE_OUTPUT_DIR': './optional_io_test_dir', 'UMPIRE_OUTPUT_BASENAME' : 'umpire_io_tests'}, 0, False, True)

    sys.exit(errors)
