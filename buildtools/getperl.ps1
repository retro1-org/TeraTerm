$PERL_URL = "http://strawberryperl.com/download/5.30.1.1/strawberry-perl-5.30.1.1-32bit.zip"
$PERL_ZIP = ($PERL_URL -split "/")[-1]
$PERL_DIR = "perl"

$PERL_ZIP = "download\perl\" + $PERL_ZIP

echo $PERL_URL
echo $PERL_ZIP
echo $PERL_DIR

# �W�J�ς݃t�H���_������?
if((Test-Path $PERL_DIR) -eq $true) {
	# �폜����
	Remove-Item -Recurse -Force $PERL_DIR
	# �I������
	#exit
}

# �_�E�����[�h����
if((Test-Path $PERL_ZIP) -eq $false) {
	if((Test-Path "download\perl") -ne $true) {
		mkdir "download\perl"
	}
	wget $PERL_URL -Outfile $PERL_ZIP
}

# �W�J����
Expand-Archive $PERL_ZIP -DestinationPath $PERL_DIR
