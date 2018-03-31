// The plan...
// To fit small printers, split the plates in 3.
// Wings are individual parts, centre has underlapping
// area, assembly screws go through both parts.
// Bottom plate has screw posts, bottom centre section loops
// around these (or wings around centre)


// Pick your print part:
leftWing();


module topPlate () {
    linear_extrude(height = 3, convexity = 10)
        import (
            file = "/home/iain/code/Keyboard/3d_Keyboard_top.dxf");
}

module leftWing() {
    intersection() {
        topPlate();
        rotate([0,0,-14])
            translate([-114,0,-2])
                cube([100,190,10],center=false);
    }
}
       