#!/usr/bin/ruby

#
# Ruby script for generating tarball releases of the Calligra repository
# This script can create signed tarballs with source code, translations and documentation
#
# SPDX-FileCopyrightText: 2017 Dag Andersen <danders@get2net.dk>
# SPDX-FileCopyrightText: 2016 Dag Andersen <danders@get2net.dk>
#
# Parts of this script is from create_tarball_kf5.rb, copyright by:
# SPDX-FileCopyrightText: 2005 Mark Kretschmann <markey@web.de>
# SPDX-FileCopyrightText: 2006-2008 Tom Albers <tomalbers@kde.nl>
# SPDX-FileCopyrightText: 2007 Angelo Naselli <anaselli@linux.it> (command line parameters)
# Some parts of this code taken from cvs2dist
#
SPDX-License-Identifier: GPL-2.0-only

require 'optparse'
require 'ostruct'
require 'find'
require 'fileutils'

# Compare two version strings
# The strings must have the format "<major>.<minor>.<release>"
# e.g "3.0.1"
# Returns:
#   1   v1 > v2
#   0   v1 == v2
#   -1  v1 < v2
def compare_versions(v1, v2)
    if v1 == "HEAD"
        return 1
    end
    vA = v1.split('.')
    vB = v2.split('.')
    if (vA[0] > vB[0])
        return 1
    end
    if (vA[0] < vB[0])
        return -1
    end
    if (vA[1] > vB[1])
        return 1
    end
    if (vA[1] < vB[1])
        return -1
    end
    if (vA[2] > vB[2])
        return 1
    end
    if (vA[2] < vB[2])
        return -1
    end
    return 0
end

# check command line parameters
options = OpenStruct.new
options.help  = false
options.sign  = true
options.program = "gpg2"
options.branch = "trunk"
options.translations = true
options.branch = "trunk"
options.docs = false
options.languages = []
options.tag = "HEAD"
options.checkversion = true
options.version = ""
options.cstring = ""
options.infolevel = 0

opts = OptionParser.new do |opts|
    opts.on_tail("-h", "--help", "Show this usage statement") do |h|
        options.help = true
    end
    opts.on("-v", "--version <version>", "Package version (Default: no version)") do |v|
        options.version = v
    end
    opts.on("-c", "--cstring <version string>", "Version string to check against version string in CMakeList.txt (Default: use version given in --version option)") do |c|
        options.cstring = c
    end
    opts.on("-n", "--no-check", "Disable version check") do |n|
        options.checkversion = false;
    end
    opts.on("-g", "--gittag <tag>", "Git tag (Default: 'HEAD')") do |g|
        options.tag = g
    end
    opts.on("-b", "--branch <tag>", "Svn branch for translations [trunk | stable] (Default: 'trunk')") do |g|
        options.tag = g
    end
    opts.on("-t", "--no-translations", "Do not include translations (Default: translations included)") do |t|
        options.translations = false
    end
    opts.on("-b", "--branch", "Translation branch [trunk/stable] (Default: 'trunk')") do |t|
        options.translations = false
    end
    #     opts.on("-d", "--docs", "TODO Include documentation (Default: docs not included)") do |d|
        # TODO
        #options.translations = true
#     end
    opts.on("-s", "--sign", "Sign tarball (Default: tarball is not signed)") do |s|
        options.sign = false
    end
    opts.on("-p", "--program <program>", "Which program to use for signing (Default: gpg2)") do |p|
        options.program = p
    end
    opts.on("-l", "--languages <language,..>", "Include comma separated list of languages only (Default: All available languages)") do |l|
        options.languages = l.split(/\s*,\s*/)
    end
    opts.on("-i", "--infolevel <level>", "Select amount of info to print during processing (0-3) (Default: 0)") do |i|
        options.infolevel = i.to_i
    end
end

begin
  opts.parse!(ARGV)
rescue Exception => e
  puts e, "", opts
  puts
  exit
end

if (options.help)
  puts
  puts opts
  puts
  exit
end

############# START #############
    
app = "calligra"

if options.checkversion
    if options.cstring.empty?
        options.cstring = options.version
    end
else
    options.cstring = "No check"
end

puts
puts "-> Processing " + app
puts  "            Git tag: #{options.tag}"
puts  "            Version: #{options.version}"
puts  "      Version check: #{options.cstring}"
puts  "             Signed: #{options.sign}"
puts  "            Program: #{options.program}"
puts  "       Translations: #{options.translations}"
puts  "             Branch: #{options.branch}"
print "          Languages: "
if options.translations
    if (options.languages.empty?)
        puts "all"
    else
        puts "#{options.languages}"
    end
else
    # no translation, so no languages
    puts
end
# TODO
# puts "      Documentation: #{options.docs}"
# puts

print "Continue? [Y/n]: "
answer = gets
answer = answer.lstrip.rstrip.chomp
if answer.empty?
    answer = "Y"
end
if not answer == "Y"
    exit
end
puts

gitdir = "calligra"
if options.version
    gitdir += "-" + options.version
end
gittar = "#{gitdir}.tar.xz"
gitsig = "#{gittar}.sig"

puts "-- Create tarball of Calligra: " + gittar

`rm -rf #{gitdir} 2> /dev/null`
if File.exist?(gittar)
    File.delete(gittar)
end
if File.exist?(gitsig)
    File.delete(gitsig)
end

Dir.mkdir(gitdir)
Dir.chdir(gitdir)

puts "-> Fetching git archive tag=#{options.tag} .."
`git archive --remote git://anongit.kde.org/calligra.git #{options.tag} | tar -x`

if !File.exist?("CMakeLists.txt")
    puts
    puts "Failed: 'git archive' failed to fetch repository"
    puts
    exit
end

# get the version
cversion=`grep '(CALLIGRA_VERSION_STRING' CMakeLists.txt | cut -d'"' -f2`
cversion = cversion.delete("\n").delete("\r").strip
if options.checkversion
    cstring = options.version
    if options.cstring
        cstring = options.cstring
    end
    if cversion != cstring
        puts
        puts "Failed: Specified version is not the same as in CMakeLists.txt"
        puts "        Specified version: '#{cstring}'"
        puts "        CMakeLists.txt   : '#{cversion}'"
        puts
        puts "        Did you forget to update version in CMakeLists.txt?"
        puts
        puts "        You can disable this test with the option: --no-check"
        puts
        exit
    end
end

# translations
if options.translations
    
    svnbase = "svn+ssh://svn@svn.kde.org/home/kde"
    # atm trunk and no revision is assumed
    if options.branch == "trunk"
        svnroot = "#{svnbase}/trunk"
    else
        svnroot = "#{svnbase}/branches/stable"
    end
    rev = ""
    
    puts "-> Fetching po file names .."
    Dir.mkdir("po")
    if FileTest.exist?("po_tmp")
        `rm -rf "po_tmp"`
    end
    Dir.mkdir("po_tmp")
    pofilenames = "po_tmp/pofilenames"
    `x=$(find $gitdir -name 'Messages.sh' | while read messagefile; do \
            if grep -q '^potfilename=' $messagefile; then \
                cat $messagefile | grep '^potfilename=' | cut -d'=' -f2 | cut -d'.' -f1; \
            fi; \
        done);\
    echo "$x" >#{pofilenames}`

    if !File.size?(pofilenames)
        puts "Failed: Could not fetch any po file names"
        exit
    end
    if options.infolevel > 0
        c = `wc -l #{pofilenames} | cut -d' ' -f1`
        puts "     Number of po file names found: " + c
    end

    puts "-> Fetching translations .."

    # get languages
    i18nlangs = `svn cat #{svnroot}/l10n-kf5/subdirs #{rev}`.split
    i18nlangsCleaned = []
    for lang in i18nlangs
        l = lang.chomp
        if !options.languages.empty?
            if options.languages.include?(l)
                i18nlangsCleaned += [l]
            end
        else l != "x-test" && !i18nlangsCleaned.include?(l)
            i18nlangsCleaned += [l]
        end
    end
    i18nlangs = i18nlangsCleaned

    if FileTest.exist?("po")
        `rm -rf "po"`
    end
    Dir.mkdir("po")
    for lang in i18nlangs
        lang.chomp!
        tmp = "po_tmp/#{lang}"
        dest = "po/#{lang}"

        # always checkout all po-files
        print "  -> Fetching #{lang} from repository ..\n"
        pofolder = "l10n-kf5/#{lang}/messages/calligra"
        if options.infolevel > 0
            `svn co #{svnroot}/#{pofolder} #{tmp}`
        else
            `svn co #{svnroot}/#{pofolder} #{tmp} 2>/dev/null`
        end

        # copy over the po-files we actually use in calligra
        File.foreach(pofilenames) do |pofile|
            pofile.chomp!
            pofilepath = "#{tmp}/#{pofile}.po"
            if !FileTest.exist?(pofilepath)
                # all files have not always been translated
                if options.infolevel > 1
                    puts "     Skipping #{pofilepath} .."
                end
                next
            end
            if !FileTest.exist?(dest)
                Dir.mkdir(dest)
            end
            if FileTest.exist?(pofilepath)
                if options.infolevel > 0
                    puts "     Copying #{pofile}.po .."
                end
                `mv #{pofilepath} #{dest}`
            end
        end
    end
    # remove temporary po dir
    `rm -rf "po_tmp"`
    
    # add l10n to compilation.
    `echo "find_package(KF5I18n CONFIG REQUIRED)" >> CMakeLists.txt`
    `echo "ki18n_install(po)" >> CMakeLists.txt`

    if options.docs
        # add docs to compilation.
        `echo "find_package(KF5DocTools CONFIG REQUIRED)" >> CMakeLists.txt`
        `echo "kdoctools_install(po)" >> CMakeLists.txt`
    end
end

# Remove cruft
`find -name ".svn" | xargs rm -rf`
Dir.chdir( ".." ) # root folder

print "-> Compressing ..  "
`tar -Jcf #{gittar} --group=root --owner=root  #{gitdir}`
puts " done."
puts ""
print "md5sum: ", `md5sum #{gittar}`
print "sha256sum: ", `sha256sum #{gittar}`

if (options.sign)
    puts "-> Signing ..  "
    `#{options.program} -a --output #{gitsig} --detach-sign #{gittar}`
    puts ""
    print "sha256sum: ", `sha256sum #{gitsig}`
end
