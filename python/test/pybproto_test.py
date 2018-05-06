import unittest
import doctest
import pybproto

class PybprotoParseTest( unittest.TestCase ):
    def test_parse_empty( self ):
        with self.assertRaises(pybproto.error):
            pybproto.parse("")

    def test_print_empty( self ):
        self.assertEqual(pybproto.new({}), "")

    def test_print_red( self ):
        self.assertEqual(pybproto.new({'red': 100}), 'R100')

