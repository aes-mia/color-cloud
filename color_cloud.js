//
// initialize
//

var canvas = document.querySelector("#c");
var gl = canvas.getContext("webgl");
if (!gl) console.log("no webgl");

var width = gl.canvas.width;
var height = gl.canvas.height;

var programInfo = webglUtils.createProgramInfo(gl, ["vertex-shader-2d", "fragment-shader-2d"]);

var Node = function(){
    this.children = [];
    this.localMatrix = m3.identity();
    this.worldMatrix = m3.identity();

    // only for clouds
    this.increasing = true;
    this.ri = 1;
    this.rmax = 5;
    this.rc = 1;
    this.v = 0.3;
    this.left = true;
};

Node.prototype.setParent = function(parent){
    if (this.parent){
        var ndx = this.parent.children.indexOf(this);
        if (ndx >= 0) this.parent.children.splice(ndx, 1);
    }
    if (parent) parent.children.push(this);
    this.parent = parent;
};

Node.prototype.updateWorldMatrix = function(parentWorldMatrix){
    if (parentWorldMatrix){
        this.worldMatrix = m3.multiply(parentWorldMatrix, this.localMatrix);
    }else{
        this.worldMatrix = this.localMatrix.slice();
    }
    var worldMatrix = this.worldMatrix;
    this.children.forEach( function(child) {
        child.updateWorldMatrix(worldMatrix);
    });
};

//
// setting the airplane
//

var airplaneArrays = {
    position: {numComponents: 2, data:
        [0, -20,
        -20, 20,
        20, 20,]
    },
    
    color: {numComponents: 4, data:
        [1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
        1.0, 1.0, 1.0, 1.0,]
    }
    
};
var airplaneInfo = webglUtils.createBufferInfoFromArrays(gl, airplaneArrays);

var airplane_pos = new Node();
airplane_pos.localMatrix = m3.identity();

var airplane = new Node();
airplane.localMatrix = m3.identity();
airplane.drawInfo = {
    uniforms: {
        u_colorOffset: [0.6, 0.6, 0, 1],
        u_colorMult: [0.4, 0.4, 0, 1],
    },
    programInfo: programInfo,
    bufferInfo: airplaneInfo,
}

// speed and angle of airplane
var speed = 4;
var angle = 50;
var a1 = angle * Math.PI / 180;;
var a2 = (angle - 90) * Math.PI / 180;;

// velocity of airplane
var vx = speed * Math.cos(a2);
var vy = - speed * Math.sin(a2);

// position of the center of airplane
var ax = 0;
var ay = 0;

airplane.setParent(airplane_pos);

// determine whether the airplane is out of screen
function out_of_screen(){
    ax = airplane_pos.worldMatrix[6];
    ay = airplane_pos.worldMatrix[7];
    if (ax < -30 || ax > width + 30 || ay < -30 || ay > height + 30) return true;
    return false;
}

//
// setting clouds
//

// constructor function that makes an array carrying cloud information (position & color)
var cloud_array = function(slices, colors){
    // slices: # of sides of cloud polygon
    // colors: (3*slices) sets of RGBA numbers

    this.position = {numComponents: 2, data: []};
    var r = 20;
    for (var i=0; i<slices; i++){
        this.position.data.push(0);
        this.position.data.push(0);
        this.position.data.push(r * Math.cos(2*Math.PI*i/20));
        this.position.data.push(r * Math.sin(2*Math.PI*i/20));
        this.position.data.push(r * Math.cos(2*Math.PI*(i+1)/20));
        this.position.data.push(r * Math.sin(2*Math.PI*(i+1)/20));
    }

    this.color = {numComponents: 4, data: []};
    for (var i=0; i<3*slices; i++){
        for (var j=0; j<3; j++)
            this.color.data.push(parseInt(colors[j], 16)/256);
        this.color.data.push(colors[3]); // A of RGBA is passed as a float
    }
};

// array containing each cloud
var clouds = [];
for (var i=0; i<100; i++){
    clouds[i] = new Node();
}

var t_count = 0;        // time counter; determines when a cloud should be added
var cloud_num = 0;      // current number of cloud at the screen
var is_left = true;     // determines the direction of a cloud

// array containing bufferInfo of clouds
var cloudInfos = [];
var palette_num = 0;
// filling cloudInfos with some palettes
{   
    // rednpink
    palette_num++;
    cloudInfos[0] = [];
    cloudInfos[0].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["EE", "45", "40", 1.0])));
    cloudInfos[0].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["C7", "27", "41", 1.0])));
    cloudInfos[0].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["80", "13", "36", 1.0])));
    cloudInfos[0].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["51", "0A", "32", 1.0])));

    // greennpink
    palette_num++;
    cloudInfos[1] = [];
    cloudInfos[1].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["8F", "B9", "A8", 1.0])));
    cloudInfos[1].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["FE", "FA", "D4", 0.9])));
    cloudInfos[1].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["FC", "D0", "BA", 1.0])));
    cloudInfos[1].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["F1", "82", "8D", 1.0]))); 
    
    // pastelpink
    palette_num++;
    cloudInfos[2] = [];
    cloudInfos[2].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["FB", "D1", "D3", 1.0])));
    cloudInfos[2].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["F1", "98", "AF", 1.0])));
    cloudInfos[2].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["EB", "B2", "D6", 1.0])));
    cloudInfos[2].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["9F", "81", "CD", 1.0])));

    // blue
    palette_num++;
    cloudInfos[3] = [];
    cloudInfos[3].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["9D", "C6", "D8", 1.0])));
    cloudInfos[3].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["00", "B3", "CA", 1.0])));
    cloudInfos[3].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["7D", "D0", "B6", 1.0])));
    cloudInfos[3].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["1D", "4E", "89", 1.0])));

    // olive
    palette_num++;
    cloudInfos[4] = [];
    cloudInfos[4].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["83", "B7", "99", 1.0])));
    cloudInfos[4].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["E2", "CD", "6D", 1.0])));
    cloudInfos[4].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["C2", "B2", "8F", 1.0])));
    cloudInfos[4].push(webglUtils.createBufferInfoFromArrays(gl, new cloud_array(20, ["E4", "D8", "B4", 1.0])));
}

var this_palette = 0;   // determine which palette to use; updated if out_of_screen is true

// returns random number in range [a, b)
function random(a, b){
    return a+Math.random()*(b-a);
}

var objects = [airplane];
var objectsToDraw = [airplane.drawInfo];

requestAnimationFrame(drawScene);

//
// rendering
//
function drawScene(time){
    time *= 0.005;

    webglUtils.resizeCanvasToDisplaySize(gl.canvas);
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
    gl.enable(gl.DEPTH_TEST);
    gl.clearColor(0.95, 0.94, 0.94, 1.0);   // may change this to white
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    //var projectionMatrix = m3.multiply(m3.projection(gl.canvas.width, gl.canvas.height), m3.translation(300, 200));
    var projectionMatrix = m3.projection(width, height);
    //to make (0, 0) be at the center, use the upper line

    airplane_pos.localMatrix = m3.multiply(m3.translation(vx, vy), airplane_pos.localMatrix);
    airplane.localMatrix = m3.rotation(-Math.atan2(vx, -vy));
    t_count++;
    
    // update clouds
    for (var i=0; i<cloud_num; i++){
        clouds[i].localMatrix = m3.multiply(m3.translation(-vx, -vy), clouds[i].localMatrix);
        if (clouds[i].left){
            clouds[i].localMatrix = m3.multiply(m3.translation(clouds[i].v * vy, clouds[i].v * (-vx)), clouds[i].localMatrix);
        }
        else{
            clouds[i].localMatrix = m3.multiply(m3.translation(clouds[i].v * (-vy), clouds[i].v * vx), clouds[i].localMatrix);
        }

        if (clouds[i].increasing && clouds[i].rc >= clouds[i].rmax) clouds[i].increasing = false;
        if (clouds[i].increasing) {
            clouds[i].localMatrix = m3.multiply(clouds[i].localMatrix, m3.scaling(1.1, 1.1));
            clouds[i].rc *= 1.1;
        }
        else {
            clouds[i].localMatrix = m3.multiply(clouds[i].localMatrix, m3.scaling(0.9, 0.9));
            clouds[i].rc *= 0.9;   
        }
    }

    // add new cloud
    if (!out_of_screen() && t_count > 3){
        t_count = 0;

        clouds[cloud_num].localMatrix = m3.identity();
        clouds[cloud_num].drawInfo = {
            uniforms: {
                u_colorOffset: [0.0, 0.0, 0, 0],
                u_colorMult: [1.0, 1.0, 1.0, 1],
            },
            programInfo: programInfo,
            bufferInfo: cloudInfos[this_palette][Math.floor(random(0, 4))],
        }
        clouds[cloud_num].setParent(airplane_pos);

        clouds[cloud_num].ri = random(5, 8);
        clouds[cloud_num].rmax = Math.max(random(8, 13), clouds[cloud_num].ri + 3);
        clouds[cloud_num].rc = clouds[cloud_num].ri;
        clouds[cloud_num].v = random(1,3) * speed / 45;
        is_left = !is_left;
        clouds[cloud_num].left = is_left;

        clouds[cloud_num].localMatrix = m3.multiply(m3.translation(-10*vx, -10*vy), m3.scaling(0.05, 0.05));
        clouds[cloud_num].localMatrix = m3.multiply(clouds[cloud_num].localMatrix, m3.scaling(clouds[cloud_num].ri, clouds[cloud_num].ri));

        objects.push(clouds[cloud_num]);
        objectsToDraw.push(clouds[cloud_num].drawInfo);
        cloud_num++;
    }

    // delete cloud
    for (var i=cloud_num-1; i>=0; i--){
        if (clouds[i].rc <= 1){
            cloud_num--;
            clouds.splice(i, 1);
            clouds.push(new Node());
        }
        
        if (objects[i+1].rc <= 1) { // objects[0] is airplane
            objects.splice(i+1, 1);
            objectsToDraw.splice(i+1, 1);
        }
    }

    airplane_pos.updateWorldMatrix();

    objects.forEach(function(object){
        object.drawInfo.uniforms.u_matrix = m3.multiply(projectionMatrix, object.worldMatrix);
    });

    // out of screen detection
    if (cloud_num == 0 && out_of_screen()){
        this_palette = Math.floor(random(0, palette_num));
        speed = random(3, 5);
        angle = random(30, 150);
        a1 = angle * Math.PI / 180;
        a2 = (angle - 90) * Math.PI / 180;

        switch(Math.floor(random(0, 4))){
            case 0: // left side
                airplane_pos.localMatrix = m3.translation(0, random(0.2, 0.8) * height);
                vx = speed * Math.cos(a2);
                vy = - speed * Math.sin(a2);
                break;
            case 1: // upper side
                airplane_pos.localMatrix = m3.translation(random(0.2, 0.8) * width, 0);
                vx = speed * Math.cos(a1);
                vy = speed * Math.sin(a1);
                break;
            case 2: // right side
                airplane_pos.localMatrix = m3.translation(width, random(0.2, 0.8) * height);
                vx = - speed * Math.cos(a2);
                vy = - speed * Math.sin(a2);
                break;
            case 3:	// lower side
                airplane_pos.localMatrix = m3.translation(random(0.2, 0.8) * width, height);
                vx = speed * Math.cos(a1);
                vy = - speed * Math.sin(a1);
                break;
        }
    }

    // drawing
    var lastUsedProgramInfo = null;
    var lastUsedBufferInfo = null;
    objectsToDraw.forEach(function(object){
        var programInfo = object.programInfo;
        var bufferInfo = object.bufferInfo;
        var bindBuffers = false;

        if (programInfo !== lastUsedProgramInfo){
            lastUsedProgramInfo = programInfo;
            gl.useProgram(programInfo.program);
            bindBuffers = true;
        }

        if (bindBuffers || bufferInfo != lastUsedBufferInfo){
            lastUsedBufferInfo = bufferInfo;
            webglUtils.setBuffersAndAttributes(gl, programInfo, bufferInfo);
        }

        webglUtils.setUniforms(programInfo, object.uniforms);
        gl.drawArrays(gl.TRIANGLES, 0, bufferInfo.numElements);
    });

    requestAnimationFrame(drawScene);
}