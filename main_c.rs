use rayon::prelude::*;
use rand::Rng;
use std::sync::Mutex;
use std::time::Instant;

// Define a 2D particle
#[derive(Debug, Clone)]
struct Particle {
    x: f64,
    y: f64,
}

// Function to get the bin index for a given particle
fn get_bin_index(p: &Particle, bin_size: f64, grid_width: usize) -> usize {
    let x_index = (p.x / bin_size).floor() as usize;
    let y_index = (p.y / bin_size).floor() as usize;
    y_index * grid_width + x_index
}

// Main function to sort particles into bins in parallel
fn main() {
    let num_particles = 1_000_000;
    let bin_size = 10.0;
    let grid_width = 100; // Assume a 1000x1000 coordinate space divided into 10x10 bins
    let grid_height = 100;
    let num_bins = grid_width * grid_height;

    // Timing the particle generation
    let start_gen = Instant::now();
    // Generate some random particles
    let particles: Vec<Particle> = (0..num_particles)
        .map(|_| {
            let mut rng = rand::thread_rng();
            Particle {
                x: rng.gen::<f64>() * 1000.0,
                y: rng.gen::<f64>() * 1000.0,
            }
        })
        .collect();
    let duration_gen = start_gen.elapsed();
    println!("Time taken to generate particles: {:?}", duration_gen);

    // Timing the binning process
    let start_bin = Instant::now();
    // Preallocate an array of bins
    let global_bins: Vec<Mutex<Vec<Particle>>> = (0..num_bins).map(|_| Mutex::new(Vec::new())).collect();

    // Parallel processing of particles
    particles.par_chunks(num_particles / rayon::current_num_threads())
        .for_each(|chunk| {
            let mut local_bins: Vec<Vec<Particle>> = vec![Vec::new(); num_bins];

            for particle in chunk {
                let bin_index = get_bin_index(particle, bin_size, grid_width);
                local_bins[bin_index].push(particle.clone());
            }

            // Merge local bins into the global bins
            for (index, bin) in local_bins.into_iter().enumerate() {
                let mut global_bin = global_bins[index].lock().unwrap();
                global_bin.extend(bin);
            }
        });
    let duration_bin = start_bin.elapsed();
    println!("Time taken to bin particles: {:?}", duration_bin);
/*
    // Timing the bin info printing
    let start_print = Instant::now();
    // Print some information about the bins
    for (index, bin) in global_bins.iter().enumerate() {
        let bin = bin.lock().unwrap();
        if !bin.is_empty() {
            let x = index % grid_width;
            let y = index / grid_width;
            println!("Bin ({}, {}) has {} particles", x, y, bin.len());
        }
    }
    let duration_print = start_print.elapsed();
    println!("Time taken to print bin information: {:?}", duration_print);
*/
    // Total time
    let total_duration = start_gen.elapsed();
    println!("Total time taken: {:?}", total_duration);
}

