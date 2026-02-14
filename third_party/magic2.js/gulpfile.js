const { src, dest, watch } = require('gulp');
const babel = require('gulp-babel');
const uglifyes = require('uglify-es');
const composer = require('gulp-uglify/composer');
const uglify = composer(uglifyes, console);
const rename = require('gulp-rename');

const taskBabel = done => {
  src('./src/magic2.es6')
    .pipe(babel({
      presets: [['@babel/preset-env', {
        targets: {
          esmodules: false,
          node: 'current',
        }
      }]],
    }).on('error', e => console.log(e)))
    .pipe(dest('.'));
  done();
};

const taskUglify = done => {
  src('./magic2.js')
    .pipe(uglify())
    .pipe(rename('./magic2-min.js'))
    .pipe(dest('.'));
  done();
};

const taskWatch = done => {
  watch('./src/magic2.es6', taskBabel);
  watch('./magic2.js', taskUglify);
};

exports.babel = taskBabel;
exports.uglify = taskUglify;
exports.watch = taskWatch;
exports.default = taskWatch;
