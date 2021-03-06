# python3

import os
import sys
import glob
import argparse
import shutil


__script_version__ = '1.0.0'


def error(msg):
    """ Display an error message and terminate. """
    msg = str(msg)
    sys.stderr.write("Error: %s\n" % msg)
    sys.exit(True)

def inform(msg):
    """ Display an information message. """
    msg = str(msg)
    sys.stdout.write("Inform: %s\n" % msg)

def newline():

    sys.stdout.write("\n")


# ------------------------------------------------------------------------------
# Enviroment
# ------------------------------------------------------------------------------
class Enviroment(object):
  """docstring for Enviroment"""
  def __init__(self, **kwargs):

    self.name = 'labsupply-stm-main'

    self.root_dir = os.path.abspath( kwargs.get('rootpath', '.') )
    self.objects_dir = os.path.abspath( kwargs.get('objectspath', '.\\.objs') )
    self.outpath = os.path.abspath( kwargs.get('outpath', '.') )
    self.verbose = kwargs.get( 'verbose', False )

  def dirs(self, **kwargs):
    """ get all filtered dirs from gived root """

    # get root dir
    root_dir = kwargs.get('root', self.root_dir)

    # create out list with root dir element
    out = [ os.path.abspath(root_dir) ]

    for _root, _dirs, _files in os.walk(root_dir):
      if '\.' not in _root:
        for d in _dirs:
          if '.' not in d:
            out.append(os.path.join(_root, d))

    return out

  def files(self, **kwargs):
    """ """

    root_dir = kwargs.get('root', self.root_dir)
    ext = kwargs.get('ext', 'False')

    out = []
    if ext:
      for _root, _dirs, _files in os.walk(root_dir):
        for f in _files:
          if f.endswith(ext):
            out.append( os.path.join(_root, f) )

    return [os.path.abspath(f) for f in out]

  def create_obj_dir(self):
    odir = os.path.abspath(self.objects_dir)
    os.makedirs(odir, mode=0x777)

  def clean(self):
    odir = os.path.abspath(self.objects_dir)
    if os.path.exists(odir):
      shutil.rmtree(odir)


# ------------------------------------------------------------------------------
# BuildBase
# ------------------------------------------------------------------------------
class BuildBase(object):
  """docstring for BuildBase"""
  def __init__(self, env):
    self.env = env

  def run(self):
    """ run """

    # prepare to compile
    self._prepare()

    # format cmd
    cmd = self._formatted_cmd()

    # inform user
    inform('Start proccess...')
    if self.env.verbose:
      inform(cmd)

    # --- run compile process, get final state
    state = os.system(cmd)

    # post compile
    self._final()


    if state != 0:
      error('Failed!')
    else:
      inform('Successfull.')


# ------------------------------------------------------------------------------
# Compiler
# ------------------------------------------------------------------------------
class Compiler(BuildBase):
  """docstring for Compiler"""
  def __init__(self, env, defs):
    super(Compiler, self).__init__(env)

    self._cmd = 'arm-none-eabi-gcc'

    self._defines = [
      'STM32F100C4',
      'STM32F10X_LD_VL',
      'STM32F100xB']
    self._defines += defs

    self._options = [
        '-mcpu=cortex-m3',
        '-mthumb',
        '-Wall',
        '-ffunction-sections',
        '-g',
        '-Os',
        '-flto',
        '-fno-builtin',
        '-c',
        # '-std=gnu99',
        # '--pedantic'
     ]

  def _prepare(self):
    """ prepare to run compiler """
    # env clean
    self.env.clean()
    # env create objects dir
    self.env.create_obj_dir()
    # change os root path to objects dir
    os.chdir(self.env.objects_dir)

  def _final(self):
    """ post compile event """
    # change os root path to user root dir
    os.chdir(self.env.root_dir)

  def _formatted_cmd(self, **kwargs):
    # cmd
    s = self._cmd + ' '
    # options
    s += ' '.join( self._options ) + ' '
    # defines
    s += ' '.join( [ '-D' + d for d in self._defines ] ) + ' '
    # includes
    s += ' '.join( [ '-I' + i for i in self.env.dirs() ] ) + ' '
    # files
    s += ' '.join( self.env.files( ext='.c' ) )
    return s


# ------------------------------------------------------------------------------
# Link
# ------------------------------------------------------------------------------
class Link(BuildBase):
  """ docstring for Link """
  def __init__(self, env):
    super(Link, self).__init__(env)

    self._cmd = 'arm-none-eabi-gcc'

    self._pre_options = [
        '-mcpu=cortex-m3',
        '-mthumb',
        '-g',
        '-nostartfiles',
        '-flto',
        '-Wl,-Map=labsupply-stm-main.map',
        '-Os',
        '-Wl,--gc-sections',

        '-L%s' % self.env.root_dir,
        '-Wl,-T%s\\arm-gcc-link.ld -g -o %s' % (self.env.root_dir, self.env.name + '.elf')
        ]

    self._post_options = ['']

    # format link file
    self.ld = self.env.root_dir + '\\arm-gcc-link.ld'
    template = self.env.root_dir + '\\link_template'

    if os.path.exists(template):

      with open(template, 'rt') as f:
        tmp = f.read()
      tmp = tmp.format(rom_start = '0x08000000')
      with open(self.ld, 'wt') as f:
        f.write(tmp)

    else:
      error('link file template % not found! Stop.' % template)


  def _prepare(self):
    """ prepare to run compiler """
    # create dest dir
    if not os.path.exists(self.env.outpath):
      os.makedirs(self.env.outpath, mode=0x777)

    # change os to dest dir
    os.chdir(self.env.outpath)

  def _final(self):
    """ post compile event """
    # change os root path to user root dir
    os.chdir(self.env.root_dir)
    os.remove(self.ld)

  def _formatted_cmd(self):
    # cmd
    s = self._cmd + ' '
    # pre options
    s += ' '.join( self._pre_options ) + ' '
    # files
    s += ' '.join( self.env.files( root=self.env.objects_dir, ext='.o' ) ) + ' '
    # post options
    s += ' '.join( self._post_options )
    return s


# ------------------------------------------------------------------------------
# GNUTools
# ------------------------------------------------------------------------------
class GNUTools(BuildBase):
  """docstring for GNUTools"""
  def __init__(self, env):
    super(GNUTools, self).__init__(env)

    self._cmd = 'arm-none-eabi-objcopy'
    self.ext = 'bin'

  def _prepare(self):
    os.chdir(self.env.outpath)
    pass

  def _final(self):
    """ post compile event """
    # change os root path to user root dir
    os.chdir(self.env.root_dir)

  def _formatted_cmd(self):

    if self.ext in 'binary':
      options = '-O binary'
    elif self.ext in 'hex':
      options = '-O ihex'
    else:
      return ''

    # cmd, options, in/out files
    s = [
      self._cmd,
      options,
      '%s.elf' % self.filepath,
      '%s.%s' % (self.filepath, self.ext),
    ]
    return ' '.join(s)

  def run(self, ext):
    self.filepath = os.path.join(self.env.outpath, self.env.name)
    self.ext = ext
    super().run()

  def readelf(self):
    newline()
    os.system('arm-none-eabi-readelf.exe -S %s.elf' % self.filepath)
    newline()

  def size(self):
    newline()
    os.system('arm-none-eabi-size.exe %s.elf' % self.filepath)
    newline()



# ------------------------------------------------------------------------------
# createArgParser
# ------------------------------------------------------------------------------
def createArgParser():
    # _prog =
    desctext = 'builder script, v%s' % __script_version__
    epitext = ('For more info visit '
               'http://hobby-research.at.ua '
               'https://github.com/LeftRadio/labsupply-stm-main')

    parser = argparse.ArgumentParser( description = desctext,
                                      epilog = epitext,
                                      prog = "python build" )

    parser.add_argument ( '-V', '--version', action='version', help = 'version',
                            version = __script_version__ )

    parser.add_argument ( '-o', '--outpath', dest = 'outpath', type = str,
                            default = os.path.abspath( os.path.dirname(__file__) ) + '\\.out',
                            metavar = '[Path]',
                            help = ('Path for output files - elf, bin, hex ' \
                                    '[default:\'.\\.out\'] ') )

    parser.add_argument( '-v', '--verbose', action='store_true', default = False,
                            help = ('increase output verbosity '
                                    '[default:False]') )

    # parser.add_argument( '--defs', dest = 'other_def', type = str,
    #                         default = None,
    #                         metavar = '[__USER_DEF__]',
    #                         help = ( 'other user defines option, '
    #                                  '[default:\'\'] ') )

    return parser

# ------------------------------------------------------------------------------
# main
# ------------------------------------------------------------------------------
def main():

    parser = createArgParser()
    args = parser.parse_args()

    # if no args
    if not len(sys.argv[1:]):
        parser.print_help()
    newline()

    # user defines
    args.verbose = True
    args.defs = []

    # inform user
    inform( 'Start with user defs - [ %s ]' % ' ; '.join(args.defs) )
    newline()

    # --- --- build

    # --- create enviroment
    envir = Enviroment( rootpath = os.path.dirname(__file__),
                        outpath = args.outpath,
                        verbose = args.verbose )

    # --- create compiler, run.
    compiler = Compiler( envir, args.defs )
    compiler.run()

    # --- create link, run.
    link = Link( envir )
    link.run()

    # --- create tools, generate bin/hex.
    tools = GNUTools( envir )
    tools.run('bin')
    tools.run('hex')

    # final, read elf
    if args.verbose:
      tools.readelf()
    tools.size()


# ------------------------------------------------------------------------------
# program start here
# ------------------------------------------------------------------------------
if __name__ == '__main__':
    sys.exit(main())