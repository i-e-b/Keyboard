// The plan...
// To fit small printers, split the plates in 3.
// Wings are individual parts, centre has underlapping
// area, assembly screws go through both parts.
// Bottom plate has screw posts, bottom centre section loops
// around these (or wings around centre)
// NOTE: the centre section should allow for pivoting,
// AND: it must have extenders for the rubber feet.
// measure up a nice cage for the teensy

$fs = 0.2;
$fa = 0.2;

// Pick your print part:
//leftWingTop();
centreTop();


module leftWingTop () {
    linear_extrude(height = 2.2, convexity = 10)
        import (
            file = "/home/iain/code/Keyboard/3d_Keyboard_wing.dxf");
}

module centreTop () {
    linear_extrude(height = 2.2, convexity = 7)
        import (
            file = "/home/iain/code/Keyboard/3d_Keyboard_centre_top.dxf");
}

module junk() {
    intersection() {
        topPlate();
        rotate([0,0,-14])
            translate([-114,0,-2])
                cube([100,190,10],center=false);
    }
}
       