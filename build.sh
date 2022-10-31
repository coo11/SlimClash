clash=$(
    curl -s 'https://api.github.com/repos/Dreamacro/clash/releases/tags/premium' | \
    python -c "import sys, json; src=json.load(sys.stdin); urls=[i['browser_download_url'] for i in src['assets'] if i['name'].find('windows-amd64') > -1]; print(src['name'].split(' ')[1] + ';' + urls[0])"
)
release_mark=$(echo $clash|cut -d ";" -f 1)
RELEASED_FILENAME=SlimClash_${release_mark// /_}.zip

curl -Loclash.zip $(echo $clash | cut -d ";" -f 2)
7z x clash.zip # clash.zip/clash-windows-amd64.exe
curl -Lodashboard.zip https://github.com/Dreamacro/clash-dashboard/archive/refs/heads/gh-pages.zip
7z x dashboard.zip
curl -LoCountry.mmdb https://github.com/Hackl0us/GeoIP2-CN/raw/release/Country.mmdb

mkdir -p SlimClash/.clash/dashboard/../ruleset/../../.utils/
mv clash-windows-amd64.exe SlimClash/clash.exe
mv clash-dashboard-gh-pages/ -T SlimClash/.clash/dashboard/
mv Country.mmdb SlimClash/.clash/country.mmdb

python parser.py
mv output.yaml SlimClash/.clash/config.yaml
cp bin/* -t SlimClash/.utils/
cp src/icon.ico SlimClash/clash.ico

7z a -mm=Deflate -mfb=258 -mpass=15 -r ${RELEASED_FILENAME} SlimClash/

RELEASE_DESC="## Base on Clash Premium ${release_mark}
### Related Link
[Original Release](https://github.com/Dreamacro/clash/releases/tag/premium)
[Dashboard Repo](https://github.com/Dreamacro/clash-dashboard)
[GeoIP2 · CN](https://github.com/Hackl0us/GeoIP2-CN)"

echo 'RELEASE_DESC<<EOF' >> $GITHUB_OUTPUT
echo "$RELEASE_DESC" >> $GITHUB_OUTPUT
echo 'EOF' >> $GITHUB_OUTPUT

echo "$RELEASED_FILENAME" >> $GITHUB_OUTPUT