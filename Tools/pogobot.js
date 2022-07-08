
exports.applyToGenerator = function (g) {
    
    g.addCodePattern(/\(/, (t) => {

        removeS = t.code.split(' ');
        removeP = removeS[1].split('(');

        t.header =  ":arrow_right: " + removeP[0];
    });

    g.addCodePattern(/^typedef/, (t) => {
    });

    
    g.addCodePattern(/^[^\(]*$/, (t) => {
        
        t.header = t.code;
    });
    
};
