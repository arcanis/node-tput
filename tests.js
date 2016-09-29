var spawnSync = require('child_process').spawnSync;
var tput = require('./');

check('setaf', 0);
check('sgr0');
check('civis');
check('clear');
check('cup', 1, 1);

function check() {

    var args = Array.prototype.slice.call(arguments);

    var expected = spawnSync('tput', args).stdout.toString().trim();
    var got = tput.apply(null, args);

    if (got !== expected) {
        console.log('Fail with ' + args.join(' '));
        console.log('  - Expected  : ' + JSON.stringify(expected));
        console.log('  - Got       : ' + JSON.stringify(got));
    }

}
