#!/bin/sh

# run the build with ntsec on if possible...
pkg=perl
ver=5.8.1
release=-2
dir=`pwd`
shortver=`echo ${ver}${release} | sed 's/-.*//'`
timestamp=${dir}/pkg.timestamp
files=${dir}/pkg.files

# cleanup
mkdir -p old/
mv -f ${timestamp} ${files} ${files}.manpages ${files}.perl ${pkg}-${ver}${release}-src.tar.bz2 ${pkg}-${ver}${release}.tar.bz2 ${pkg}_manpages-${ver}${release}.tar.bz2 ${pkg}-${ver}${release}/ old/
mv -f /usr/share/doc/Cygwin/${pkg}-${shortver}.README old
# cleanup modules
for i in Archive-Tar-1.05	\
         Compress-Zlib-1.22	\
         MD5-2.02	\
         Term-ReadLine-Perl-1.0203 \
         Net-Telnet-3.03	\
         TermReadKey-2.21;	\
do rm -rf ${i};	\
done


# start
set -e
touch ${timestamp}
touch README

## install READMEs
/bin/install README /usr/share/doc/Cygwin/${pkg}-${shortver}.README 
mkdir -p /usr/share/doc/${pkg}-${ver}${release}
rm -rf /usr/share/doc/${pkg}-${ver}${release}/Artistic /usr/share/doc/${pkg}-${ver}${release}/Copying /usr/share/doc/${pkg}-${ver}${release}/README

# build perl now
sh -x ${dir}/build.sh

# source package
rm -rf ${pkg}-${ver}${release}
mkdir ${pkg}-${ver}${release}
cp -p README *.patch *.pl *.sh *.tar.gz *.tar.bz2 *_debug ${pkg}-${ver}${release}
for i in `ls log.*`; do cp ${i} ${pkg}-${ver}${release}/${i}.rel; done
tar cjf ${pkg}-${ver}${release}-src.tar.bz2 ${pkg}-${ver}${release}

# binary package
# first the files
(cd / ; find usr/share/man -path usr/lib/perl5 -prune -o -type f -newer ${timestamp} -print) >${files}
(cd / ; find usr/bin -path usr/lib/perl5 -prune -o -type f -newer ${timestamp} -print) >>${files}
(cd / ; find usr/share/doc -path usr/lib/perl5 -prune -o -type f -newer ${timestamp} -print) >>${files}
echo usr/lib/perl5/${shortver} >>${files}
echo usr/lib/perl5/site_perl/${shortver} >>${files}
echo usr/share/doc/${pkg}-${ver}${release} >>${files}
# separate manpages and remove builddirs from list
${dir}/separate.pl
# then pack it up
(cd /;tar -c -T ${files}.manpages) | bzip2 -9 >${pkg}_manpages-${ver}${release}.tar.bz2
(cd /;tar -c -T ${files}.perl) | bzip2 -9 >${pkg}-${ver}${release}.tar.bz2