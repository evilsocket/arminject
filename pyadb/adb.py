# Copyright (c) 2015, Simone Margaritelli <evilsocket at gmail dot com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#   * Neither the name of ARM Inject nor the names of its contributors may be used
#     to endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import subprocess
import time
import os

class ADB:
    def __init__(self):
        # make sure we have adb
        if self._exec( "which adb" ) == "":
            raise "ADB binary not found in PATH."

    def push( self, src, dst ):
        self._exec( "adb push '%s' '%s'" % ( src, dst ), True )

    def sh( self, cmd ):
        return self._exec( "adb shell '%s'" % cmd )

    def sudo( self, cmd ):
        return self.sh( "su -c \"%s\"" % cmd )

    def pkill( self, proc ):
        self.sudo( "pkill -9 %s" % proc )

    def clear_log( self ):
        self._exec( "adb logcat -c" )

    def set_selinux_level( self, level ):
        self.sh( "su 0 setenforce %d" % level )
        if level == 0:
            self.sh( 'su -c supolicy --live "allow s_untrusted_app shell_data_file file { execute execute_no_trans }"' )

    def get_pid( self, proc ):
        out = self.sudo( "ps | grep '%s'" % proc ).strip().split(' ')
        out = [x for x in out if x]
        pid = int( out[1] )
        return pid

    def start_activity( self, proc, activity ):
        self.pkill( proc )
        time.sleep(1)
        self.sh( "am start %s/%s" % ( proc, activity ) )
        return self.get_pid( proc )

    def logcat( self, tag = None ):
        if tag is not None:
            cmd = "adb logcat -s '%s'" % tag
        else:
            cmd = "adb logcat"

        proc = subprocess.Popen( cmd, stdout=subprocess.PIPE, shell=True )
        for line in iter( proc.stdout.readline, '' ):
            print line.rstrip()

    def _exec( self, cmdline, silent = False ):
        channel = open(os.devnull, 'wb') if silent is True else subprocess.PIPE
        p = subprocess.Popen( cmdline, stdout=channel, stderr=channel, shell=True )
        out, err = p.communicate()
        if err:
            print err
        if p.returncode != 0:
            print "[STDERR] : %s" % out

        return out
