import subprocess, os
from pygments.lexer import RegexLexer
from pygments import token
from sphinx.highlighting import lexers

# TODO: Update this syntax highlighter
class AngelScriptLexer(RegexLexer):
    name = 'AngelScript'

    tokens = {
        'root': [
            (r'int', token.Keyword),
            (r'float', token.Keyword),
            (r'double', token.Keyword),
            (r'const', token.Keyword),
            (r'return', token.Keyword),
            (r'null', token.Literal),
            (r'\+', token.Operator),
            (r'-', token.Operator),
            (r'\*', token.Operator),
            (r'/', token.Operator),
            (r'<', token.Operator),
            (r'>', token.Operator),
            (r'&', token.Operator),
            (r'@', token.Operator),
            (r'\(', token.Punctuation),
            (r'\)', token.Punctuation),
            (r'\[', token.Punctuation),
            (r'\]', token.Punctuation),
            (r'\{', token.Punctuation),
            (r'\}', token.Punctuation),
            (r'"', token.Punctuation),
            (r"'", token.Punctuation),
            (r';', token.Punctuation),
            (r',', token.Punctuation),
            (r'[a-zA-Z]', token.Name),
            (r'\s', token.Text)
        ]
    }

lexers["AngelScript"] = AngelScriptLexer(startinline=True)
lexers["AngelScript"].aliases = ["as", "angelscript"]

# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'asbind20'
copyright = '2024, HenryAWE'
author = 'HenryAWE'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [ "breathe" ]

templates_path = ['_templates']
exclude_patterns = []



# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']

# Breathe Configuration
breathe_projects = {}
breathe_default_project = "asbind20"

# Check if we're running on Read the Docs' servers
read_the_docs_build = os.environ.get('READTHEDOCS', None) == 'True'

if read_the_docs_build:
    os.mkdir("doxygen-output")
    subprocess.call("doxygen", shell=True)
    breathe_projects["asbind20"] = "doxygen-output/xml"
else:
    breathe_projects["asbind20"] = "../build/doxygen-output/xml"
