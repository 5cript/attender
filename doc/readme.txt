How to generate wiki documentation:

current directory = attender/doc

Setup:
- npm install

Run Doxygen:
- doxygen Doxyfile	

Clone attender wiki:
- mkdir md
- cd md
- git clone git@github.com:5cript/attender.wiki.git .
- cd ..

Generate markdown:
- (remove previous markdown files if necessary from md subdirectory, but only generated ones!!!)
- node generate.js

Update wiki:
- cd md
- git add -u
- git commit -m "SOMETHING"
- git push