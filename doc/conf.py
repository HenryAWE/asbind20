import subprocess, os, sys

sys.path.insert(0, os.path.abspath('.'))

# Project information

project = 'asbind20'
copyright = '2024, HenryAWE'
author = 'HenryAWE'

# General configuration
extensions = [ 'breathe' ]

templates_path = ['_templates']
exclude_patterns = []

# Options for HTML output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Breathe Configuration
breathe_projects = {}
breathe_default_project = 'asbind20'

# Check if we're running on Read the Docs' servers
read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    os.mkdir('doxygen-output')
    subprocess.call('doxygen', shell=True)
    breathe_projects['asbind20'] = 'doxygen-output/xml'
else:
    breathe_projects['asbind20'] = '../build/doxygen-output/xml'

from pygments.lexer import RegexLexer, bygroups, include, default, words
from pygments.token import Text, Comment, String, Keyword, Name, Operator, \
    Punctuation, Number, Literal, Generic

# Converted from https://github.com/wronex/sublime-angelscript/blob/master/AngelScript.tmLanguage by DeepSeek-V3
# and adapted by HenryAWE

class AngelScriptLexer(RegexLexer):
    """
    For AngelScript source code.
    """

    tokens = {
        'root': [
            (r'[ \t\n]+', Text),

            # Comments
            (r'//.*?$', Comment.Single),
            (r'/\*.*?\*/', Comment.Multiline),

            # Strings
            (r'""".*?"""', String.Double),
            (r'"(\\\\|\\"|[^"])*"', String.Double),
            (r"'(\\\\|\\'|[^'])*'", String.Single),

            # Keywords
            (words((
                'for', 'foreach', 'in', 'break', 'continue', 'while', 'do',
                'return', 'if', 'else', 'case', 'switch', 'namespace', 'try',
                'catch', 'import', 'from'), suffix=r'\b'),
             Keyword.Control),
            (words((
                'enum', 'void', 'bool', 'typedef', 'funcdef', 'int', 'int8',
                'int16', 'int32', 'int64', 'uint', 'uint8', 'uint16', 'uint32',
                'uint64', 'string', 'ref', 'array', 'double', 'float', 'auto',
                'dictionary'), suffix=r'\b'),
             Keyword.Type),
            (words((
                'get', 'in', 'inout', 'out', 'override', 'explicit', 'set',
                'private', 'public', 'protected', 'const', 'default', 'final',
                'shared', 'external', 'mixin', 'abstract', 'property'), suffix=r'\b'),
             Keyword.Declaration),
            (words(('null', 'true', 'false'), suffix=r'\b'), Literal.Constant),
            (words(('this', 'super'), suffix=r'\b'), Name.Builtin),
            (words(('is', 'cast'), suffix=r'\b'), Keyword),
            (words(('or', 'and', 'xor', 'not'), suffix=r'\b'), Operator.Word),

            # Operators
            (r'[=]', Operator),
            (r'[%*+\-/]', Operator),
            (r'[|&><]', Operator),
            (r'[!@?:]', Operator),
            (r'(~|!|&&|\|\|)', Operator),
            (r'::', Operator),

            # Numbers
            (r'\b0[xX][0-9a-fA-F]+\b', Number.Hex),
            (r'\b\d+\.\d*([eE][+-]?\d+)?[fF]?\b', Number.Float),
            (r'\b\.\d+([eE][+-]?\d+)?[fF]?\b', Number.Float),
            (r'\b\d+[fF]\b', Number.Float),
            (r'\b\d+[uU][lL]?\b', Number),
            (r'\b\d+\b', Number.Integer),

            # Preprocessor
            (r'^\s*#\w+', Comment.Preproc),

            # Metadata
            (r'^\s*\[.*?\]', Name.Decorator),

            # Class definitions
            (r'\b(class|interface)\b([ \t]+)([a-zA-Z_][a-zA-Z0-9_]*)'
             r'(?:\s*:\s*([a-zA-Z_][a-zA-Z0-9_]*)'
             r'(?:\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:\s*,\s*([a-zA-Z_][a-zA-Z0-9_]*))?)?',
             bygroups(Keyword.Declaration, Name.Class, Name.Class, Name.Class, Name.Class, Name.Class)),

            # Function definitions
            (r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(',
             bygroups(Keyword.Type, Name.Function)),

            # Cast operator
            (r'\b(cast)\s*(<)\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*(>)',
             bygroups(Keyword, Punctuation, Keyword.Type, Punctuation)),

            # Variables with @
            (r'([a-zA-Z_][a-zA-Z0-9_]*)(@)([ \t]+)([a-zA-Z_][a-zA-Z0-9_]*)',
             bygroups(Keyword.Type, Operator, Text, Name.Variable)),
            (r'([a-zA-Z_][a-zA-Z0-9_]*)(@)',
             bygroups(Keyword.Type, Operator)),
            (r'@([a-zA-Z_][a-zA-Z0-9_]*)', Name.Variable),

            # Variable declarations
            (r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[=;]',
             bygroups(Keyword.Type, Name.Variable)),

            # Template variable declarations
            (r'^\s*([a-zA-Z_][a-zA-Z0-9_]*)'
             r'<([a-zA-Z_][a-zA-Z0-9_]*)'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?>'
             r'\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[=;]',
             bygroups(Keyword.Type, Keyword.Type, Keyword.Type, Keyword.Type, Keyword.Type, Keyword.Type, Name.Variable)),

            # Function calls
            (r'(?:\b|\.)([a-zA-Z_][a-zA-Z0-9_]*)\b(?:\s*\()',
             Name.Function),

            # Template function calls
            (r'(?:\b|\.)([a-zA-Z_][a-zA-Z0-9_]*)'
             r'<([a-zA-Z_][a-zA-Z0-9_]*)'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?'
             r'(?:,\s*([a-zA-Z_][a-zA-Z0-9_]*))?>\(',
             bygroups(Name.Function, Keyword.Type, Keyword.Type, Keyword.Type, Keyword.Type, Keyword.Type)),

            # Constants
            (r'\b([A-Z][A-Z0-9_]+)\b', Name.Constant),

            # Dot access
            (r'\.[a-zA-Z_][a-zA-Z0-9_]*\b(?!\s*[<\(])', Name.Attribute),

            # Identifiers
            (r'[a-zA-Z_][a-zA-Z0-9_]*', Name),

            # Punctuation
            (r'[\[\](){};,:]', Punctuation),

            # Assignment operators
            (r'\b([a-zA-Z_][a-zA-Z0-9_]*)\s*([-+/*%]?=)',
             bygroups(Name.Variable, Operator)),
        ]
    }

from sphinx.highlighting import lexers

lexers['AngelScript'] = AngelScriptLexer(startinline=True)
lexers['AngelScript'].aliases = ['angelscript', 'as']
