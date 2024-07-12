use rand::Rng;
use std::time::Instant;

const NUM_PARTICLES: usize = 1_000_000;
const BIN_SIZE: f64 = 10.0;
const GRID_WIDTH: usize = 100;
const GRID_HEIGHT: usize = 100;
const MAX_PARTICLES_PER_BIN: usize = 256;

#[derive(Debug, Clone, Copy)]
struct Particle {
    x: f64,
    y: f64,
}

#[derive(Debug)]
struct Bin {
    particles: Box<[Particle; MAX_PARTICLES_PER_BIN]>,
    count: usize,
}

impl Bin {
    fn new(empty_particle: Particle) -> Self {
        Bin {
            particles: Box::new([empty_particle; MAX_PARTICLES_PER_BIN]),
            count: 0,
        }
    }

    fn add_particle(&mut self, particle: Particle) {
        if self.count < MAX_PARTICLES_PER_BIN {
            self.particles[self.count] = particle;
            self.count += 1;
        }
    }
}

// Function to get the bin index for a given particle
fn get_bin_index(x: f64, y: f64) -> usize {
    let x_index = (x / BIN_SIZE).floor() as usize;
    let y_index = (y / BIN_SIZE).floor() as usize;
    y_index * GRID_WIDTH + x_index
}

fn main() {
    // Allocate particles on the heap
    let mut particles: Vec<Particle> = vec![Particle { x: 0.0, y: 0.0 }; NUM_PARTICLES];

    // Define an "empty" particle
    let empty_particle = Particle { x: f64::NAN, y: f64::NAN };

    // Allocate bins on the heap
    let mut bins: Vec<Bin> = Vec::with_capacity(GRID_WIDTH * GRID_HEIGHT);
    for _ in 0..(GRID_WIDTH * GRID_HEIGHT) {
        bins.push(Bin::new(empty_particle));
    }

    // Timing the particle generation
    let start_gen = Instant::now();
    // Generate some random particles
    let mut rng = rand::thread_rng();
    for i in 0..NUM_PARTICLES {
        particles[i] = Particle {
            x: rng.gen::<f64>() * 1000.0,
            y: rng.gen::<f64>() * 1000.0,
        };
    }
    let duration_gen = start_gen.elapsed();
    println!("Time taken to generate particles: {:?}", duration_gen);

    // Timing the binning process
    let start_bin = Instant::now();

    for particle in &particles {
        let bin_index = get_bin_index(particle.x, particle.y);
        bins[bin_index].add_particle(*particle);
    }

    let duration_bin = start_bin.elapsed();
    println!("Time taken to bin particles: {:?}", duration_bin);
/*
    // Timing the bin info printing
    let start_print = Instant::now();
    // Print some information about the bins
    for (i, bin) in bins.iter().enumerate() {
        if bin.count > 0 {
            let x = i % GRID_WIDTH;
            let y = i / GRID_WIDTH;
            println!("Bin ({}, {}) has {} particles", x, y, bin.count);
        }
    }
    let duration_print = start_print.elapsed();
    println!("Time taken to print bin information: {:?}", duration_print);
*/
    // Total time
    let total_duration = start_gen.elapsed();
    println!("Total time taken: {:?}", total_duration);
}

