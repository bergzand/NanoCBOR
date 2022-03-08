#!/usr/bin/env python3
import json
import pexpect
import base64
import binascii
import argparse
import re
import os
import os.path

TEST_SKIPPED = [
        "c249010000000000000000",
        "3bffffffffffffffff", # negative max, can't represent in int64_t
        "c349010000000000000000", # Negative bignum
        "c1fb41d452d9ec200000", # Float as epoch type
        "5f42010243030405ff", # Indefinite length byte string
        "7f657374726561646d696e67ff", # Indefinite length text string
        "62225c", # Encoding mumbo jumbo
        "fa7f7fffff"
        ]

#FLOAT_MATCH = re.compile(r'\d+(.\d+)?')
FLOAT_MATCH = re.compile(r'^-?(\d+(.\d+)?|inf|nan)\s?$', re.MULTILINE)

def main(script_dir, printer):
    data = ""
    for vector_file in os.scandir(script_dir):
        if vector_file.name.endswith('.json') and vector_file.is_file():
            test_file(vector_file, printer)

def test_file(vector_file, printer):
    with open(vector_file, 'r') as f:
        appendix = json.load(f)
    for test_case in appendix:
        if test_case['hex'] in TEST_SKIPPED:
            print(f"skipping input: {test_case['hex']}")
            continue
        test_input = base64.b64decode(test_case["cbor"])

        print(f"input: {test_case['hex']}")
        test_run = pexpect.spawn(f'bash -c "base64 -d <<< {test_case["cbor"]} |\
                {printer} -f-"', encoding='utf-8', echo=False)
        bytes_written = len(test_input)
        test_run.expect_exact(f"Start decoding {bytes_written} bytes:")

        float_test = False
        if "diagnostic" in test_case:
            test_string = str(test_case["diagnostic"])
        else:
            test_string = json.dumps(test_case["decoded"], ensure_ascii=False)
        try:
            test_float = float(test_string)
            float_test = True
            int(test_string)
            float_test = False
        except ValueError:
            pass
        if float_test:
            matches = test_run.expect(FLOAT_MATCH)
            float_match = float(test_run.match.group(matches))
            print(f"Comparing {float_match} to {test_float}")
            try:
                assert(float_match == test_float)
            except AssertionError as e:
                print(e)
        else:
            test_run.expect_exact(test_string)
        output = test_run.read()
        test_run.wait()
        print(f"input {test_case['hex']}, bytes {bytes_written}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
            description='Test harness for RFC test vectors')
    parser.add_argument('printer', type=str,
                        help='Path to the pretty printer application')
    args = parser.parse_args()
    script_dir = os.path.dirname(os.path.realpath(__file__))
    main(script_dir, args.printer)
