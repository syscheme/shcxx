# Subtree in ZQProjs

```
cd ZQProjs
git remote add --fetch shcxx https://github.com/syscheme/shcxx.git
git subtree add --prefix=Common shcxx/zqrel3.5 --squash
git subtree pull --prefix=Common
ls Common/
```
make some changes and commit to ZQ gitlab
```
cd Common/
echo -e "\n\n" >> Text.h 
git status
git commit -m "test checking" -a
git push
```
if want to push changes to shcxx as well, or archived a punch of changes, then
```
cd ..
git subtree push --prefix=Common shcxx zqrel3.5
```
