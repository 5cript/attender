var doxy2md = require('doxygen2md')
var fs = require('fs')

var mkdirSync = function (path) {
  try {
    fs.mkdirSync(path);
  } catch(e) {
    if ( e.code != 'EEXIST' ) throw e;
  }
}

const makeOptions = () => {
    var options = doxy2md.defaultOptions
    options.directory = './xml'
    options.anchors = false
    options.templatePath = './templates'

    options.writer = (content) => {
        var lines = content.split('\n')
        if (lines.length > 0)
        {
            var fileName = './md/' + lines[0]
            lines.splice(0, 1)
            fs.writeFileSync(fileName, lines.join('\n'), {'flag': 'w'})
        }
        else
        {
            if (options._file)
                fs.writeSync(options._file, content)
            else
                process.stdout.write(content)
        }
    }

    options.cb = () => {
        if (options._file)
            fs.closeSync(options._file)
    }

    return options
}

const render = (options) => {
    doxy2md.render(options)
}

mkdirSync('./md')
render(makeOptions())

fs.copyFileSync('./special_files/Home.md', './md/Home.md');
fs.copyFileSync('./special_files/_Sidebar.md', './md/_Sidebar.md');

var regex = /(\(#\w*\))/ // classattender_1_1request__handler -> class-request_handler
