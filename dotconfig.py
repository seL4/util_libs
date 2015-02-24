#!/usr/bin/env python

'''
Python functionality for manipulating a Kconfig .config file.

The Kconfig build configuration system produces .config files that are
essentially conf format (a.k.a. INI) with no sections. When editing multiple
such files, usually some combination of sed and awk are enough, but
occasionally you need more advanced programmatic editing. This file provides an
object, DotConfig, for reading and then editing .config files programmatically.

The implementation is somewhat more involved than a simple conf parser and dict
because we try to preserve line ordering as much as possible. This makes for
cleaner diffs and allows straightforward in-place edits. The interface also
gives the user control over the timestamp header if they need to preserve or
update this.

To use this from the command-line:
 # Set CONFIG_FOO=y
 dotconfig.py --set FOO=y .config

 # Unset CONFIG_BAR
 dotconfig.py --unset BAR .config

 # Print the value of CONFIG_BAZ
 dotconfig.py --get BAZ .config

And the same programmatically:
 import dotconfig

 c = dotconfig.DotConfig('.config')
 c['FOO'] = True
 c['BAR'] = None
 print c['BAZ']
 c.save()

'''

import argparse, datetime, os, re, sys

# Timestamp format used in .config files.
TIMESTAMP_FORMAT = '%a %b %d %H:%M:%S %Y'

class Line(object):
    pass

class Uninterpreted(Line):
    '''
    A line in a .config file that we know nothing about. Assume this must be
    emitted exactly as it was seen.
    '''
    def __init__(self, content):
        self.content = content
    def __repr__(self):
        return self.content

class Timestamp(Line):
    '''
    A timestamp line in a .config file. Not generally interpreted, but
    occasionally the user will want control over this. See references to
    preserve_time below for how to control this.
    '''
    def __init__(self, timestamp):
        self.timestamp = timestamp
    def __repr__(self):
        return '# %s' % self.timestamp.strftime(TIMESTAMP_FORMAT)
        
class Option(Line):
    '''
    A Kconfig option value or unset option. These will generally be the lines
    users want to manipulate.
    '''
    def __init__(self, key, value):
        self.key = key
        self.value = value
    def __repr__(self):
        if self.value is None or (isinstance(self.value, bool) and not self.value):
            return '# CONFIG_%s is not set' % self.key
        elif isinstance(self.value, bool):
            assert self.value
            return 'CONFIG_%s=y' % self.key
        elif isinstance(self.value, int):
            return 'CONFIG_%s=%d' % (self.key, self.value)
        elif isinstance(self.value, str):
            return 'CONFIG_%s="%s"' % (self.key, self.value)
        raise NotImplementedError

def parse_value(raw):
    '''
    Parse an expression that appears in the assignment of a setting
    ('CONFIG_FOO=%s').
    '''
    if raw == 'y':
        # Boolean setting that is enabled.
        return True

    elif raw.startswith('"') and raw.endswith('"'):
        # String setting.
        return raw[1:-1]

    else:
        try:
            return int(raw)
        except ValueError:
            # Something else that we don't currently support.
            raise NotImplementedError

line_regex = None
def parse_line(line):

    # The first time this parser is called we construct this regex on demand
    # because it is slightly expensive.
    global line_regex
    if line_regex is None:
        line_regex = re.compile(r'(?P<option1>CONFIG_(?P<key1>[A-Za-z][A-Za-z0-9_]*)=(?P<value>.*))|' \
                                r'(?P<option2># CONFIG_(?P<key2>[A-Za-z][A-Za-z0-9_]*) is not set)')

    match = line_regex.match(line)

    if match is not None:
        # This line is one of the option forms.

        if match.group('option1') is not None:
            # A set option.

            key = match.group('key1')
            raw_value = match.group('value')

            value = parse_value(raw_value)

            return Option(key, value)

        else:
            # An unset option.
            assert match.group('option2') is not None
            key = match.group('key2')
            return Option(key, None)

    else:

        try:
            timestamp = datetime.datetime.strptime(line[2:], TIMESTAMP_FORMAT)
            # If that succeeded, we have a timestamp line.
            return Timestamp(timestamp)

        except ValueError:
            # Some generic line that we'll handle below.
            pass

        return Uninterpreted(line)

class DotConfig(object):
    '''
    A .config file. This is essentially just a glorified dictionary, but it
    takes some care to preserve the ordering of the input lines where possible.
    Callers can either interact with an object of this type through the dict
    interface, or by directly accessing 'options' and 'lines' if they need an
    explicit line ordering.
    '''

    def __init__(self, path):
        self.path = os.path.abspath(path)

        # We'll track the .config lines that represent actual options in a dict
        # as well as the list of lines to give the user more natural syntax for
        # getting and setting them.
        self.options = {}

        # Also duplicate the tracking of timestamps so we can easily update
        # them in __setitem__ below.
        self.timestamps = []

        # Read and parse the config file.
        self.lines = []
        with open(path, 'r') as f:
            for line in f:
                line = line[:-1] # Strip trailing newline.
                l = parse_line(line)
                self.lines.append(l)
                if isinstance(l, Timestamp):
                    self.timestamps.append(l)
                elif isinstance(l, Option):
                    # Enable dictionary lookup of this option later.
                    self.options[l.key] = l

        self.preserve_time = False

    def save(self):
        with open(self.path, 'w') as f:
            print >>f, str(self)

    def __getitem__(self, key):
        return self.options[key].value

    def __setitem__(self, key, value):
        if key in self.options:
            # Update the option in-place to preserve line ordering.
            self.options[key].value = value
        else:
            opt = Option(key, value)
            self.lines.append(opt)
            self.options[opt.key] = opt
        if not self.preserve_time:
            for t in self.timestamps:
                t.timestamp = datetime.datetime.now()

    def __delitem__(self, key):
        opt = self.options[key]
        self.lines.remove(opt)
        del self.options[key]

    def __item__(self):
        return iter(self.options)

    def __len__(self):
        return len(self.options)

    def __repr__(self):
        return '\n'.join([str(x) for x in self.lines])

def main(argv, out, err):
    parser = argparse.ArgumentParser(prog=argv[0],
        description='Kconfig configuration file manipulator')
    parser.add_argument('--set', '-s', action='append', default=[],
        help='set a configuration setting')
    parser.add_argument('--unset', '-u', action='append', default=[],
        help='unset a configuration setting')
    parser.add_argument('--get', '-g', action='append', default=[],
        help='print the value of a setting')
    parser.add_argument('--preserve-timestamp', action='store_true',
        help='keep the current timestamp in the .config file')
    parser.add_argument('file', help='path to .config')

    opts = parser.parse_args(argv[1:])

    try:
        c = DotConfig(opts.file)
    except Exception as e:
        print >>err, 'failed to parse %s: %s' % (opts.file, e)
        return -1

    if opts.preserve_timestamp:
        c.preserve_time = True

    for s in opts.set:
        if '=' in s:
            key, raw_value = s.split('=', 1)
            try:
                value = parse_value(raw_value)
            except NotImplementedError:
                print >>err, 'unsupported value %s in --set argument %s' % \
                    (raw_value, s)
                return -1
            c[key] = value
        else:
            c[s] = True

    for s in opts.unset:
        c[s] = None

    c.save()

    for s in opts.get:
        try:
            print >>out, c.options[s]
        except KeyError:
            print >>err, 'setting %s not found' % s
            return -1

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv, sys.stdout, sys.stderr))
