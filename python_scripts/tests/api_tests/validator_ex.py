import os
import sys
import traceback
import string
import ast
import json
import logging

# Python 3 compatibility
PYTHON_MAJOR_VERSION = sys.version_info[0]
if PYTHON_MAJOR_VERSION > 2:
    from past.builtins import basestring
    from past.builtins import long

try:  # First try to load pyresttest from global namespace
   from pyresttest import validators
   from pyresttest import parsing
   from pyresttest import six
except ImportError:  # Then try a relative import if possible
   from .. import validators
   from .. import parsing
   from .. import six

# Python 3 compatibility shims
from six import binary_type
from six import text_type


def dict_str_eq(x, y):
   """ Check if dict object is equal to string object """
   assert(isinstance(x, dict))
   assert(isinstance(y, str))
   y = ast.literal_eval(y)
   assert(isinstance(y, dict))
   return x == y

COMPARATORS = { 'dict_str_eq': dict_str_eq }


PATTERN_FILE_EXT = ".json.pat"
OUTPUT_FILE_EXT = ".json.out"

def dump_output(output_file_name, output):
   """ Dump JSON output to the file. """
   try:
      with open(output_file_name, "w") as f:
         json.dump(output, f, sort_keys=True, indent=4)
   except Exception as e:
      logging.error("Cannot dump output to file {0}.".format(output_file_name))
      logging.info(traceback.format_exc())


class JSONFileValidator(validators.AbstractValidator):
    """ Does extract response body and compare with given my_file_name.pat.json.
        If comparison failed response is save into my_file_name.out.json file.
    """

    name = 'ComparatorValidator'
    config = None   # Configuration text, if parsed
    extractor = None
    comparator = None
    comparator_name = ""
    expected = None
    isTemplateExpected = False

    def get_readable_config(self, context=None):
        """ Get a human-readable config string """
        string_frags = list()
        string_frags.append(
            "Extractor: " + self.extractor.get_readable_config(context=context))
        if isinstance(self.expected, validators.AbstractExtractor):
            string_frags.append("Expected value extractor: " +
                                self.expected.get_readable_config(context=context))
        elif self.isTemplateExpected:
            string_frags.append(
                'Expected is templated, raw value: {0}'.format(self.expected))
        return os.linesep.join(string_frags)

    def validate(self, body=None, headers=None, context=None):
        try:
            extracted_val = self.extractor.extract(
                body=body, headers=headers, context=context)
        except Exception as e:
            trace = traceback.format_exc()
            return validators.Failure(message="Extractor threw exception", details=trace, validator=self, failure_type=validators.FAILURE_EXTRACTOR_EXCEPTION)

        # Compute expected output, either templating or using expected value
        file_name = None
        if self.isTemplateExpected and context:
            file_name = string.Template(
                self.expected).safe_substitute(context.get_values())
        else:
            file_name = self.expected

        expected_val = None
        expected_file_name = file_name + PATTERN_FILE_EXT
        output_file_name = file_name + OUTPUT_FILE_EXT
        try:
           with open(expected_file_name, "r") as f:
              expected_val = json.load(f)
        except Exception as e:
           trace = traceback.format_exc()
           dump_output(output_file_name, extracted_val)
           return validators.Failure(message="Cannot load pattern file {0}.".format(expected_file_name), details=trace, validator=self, failure_type=validators.FAILURE_VALIDATOR_EXCEPTION)

        # Handle a bytes-based body and a unicode expected value seamlessly
        if isinstance(extracted_val, binary_type) and isinstance(expected_val, text_type):
            expected_val = expected_val.encode('utf-8')
        comparison = self.comparator(extracted_val, expected_val)

        if not comparison:
            failure = validators.Failure(validator=self)
            failure.message = "Comparison failed, evaluating {0}({1}, {2}) returned False".format(
                self.comparator_name, extracted_val, expected_val)
            failure.details = self.get_readable_config(context=context)
            failure.failure_type = validators.FAILURE_VALIDATOR_FAILED
            dump_output(output_file_name, extracted_val)
            return failure
        else:
            return True

    @staticmethod
    def parse(config):
        """ Create a validator that does an extract from body and applies a comparator,
            Then does comparison vs expected value
            Syntax sample:
              { jsonpath_mini: 'node.child',
                operator: 'eq',
                expected: 'my_file_name'
              }
        """

        output = JSONFileValidator()
        config = parsing.lowercase_keys(parsing.flatten_dictionaries(config))
        output.config = config

        # Extract functions are called by using defined extractor names
        output.extractor = validators._get_extractor(config)

        if output.extractor is None:
            raise ValueError(
                "Extract function for comparison is not valid or not found!")

        if 'comparator' not in config:  # Equals comparator if unspecified
            output.comparator_name = 'eq'
        else:
            output.comparator_name = config['comparator'].lower()
        output.comparator = validators.COMPARATORS[output.comparator_name]
        if not output.comparator:
            raise ValueError("Invalid comparator given!")

        try:
            expected = config['expected']
        except KeyError:
            raise ValueError(
                "No expected value found in comparator validator config, one must be!")

        # Expected value can be base or templated string contains file name without extension.
        if isinstance(expected, basestring) or isinstance(expected, (int, long, float, complex)):
            output.expected = expected
        elif isinstance(expected, dict):
            expected = parsing.lowercase_keys(expected)
            template = expected.get('template')
            if template:  # Templated string
                if not isinstance(template, basestring):
                    raise ValueError(
                        "Can't template a comparator-validator unless template value is a string")
                output.isTemplateExpected = True
                output.expected = template
            else:  # Extractor to compare against
                raise ValueError(
                    "Can't supply a non-template, non-extract dictionary to comparator-validator")

        return output

VALIDATORS = {
   'json_file_validator': JSONFileValidator.parse,
   'json_file_validate': JSONFileValidator.parse }
