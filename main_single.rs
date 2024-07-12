use hashbrown::HashMap;
use rand::Rng;
use std::time::Instant;

// Define a 2D particle
#[derive(Debug, Clone)]
struct Particle {
    x: f64,
    y: f64,
    // You can add more properties here
}

// Function to get the bin key for a given particle
fn get_bin_key(p: &Particle, bin_size: f64) -> (i64, i64) {
    (
        (p.x / bin_size).floor() as i64,
        (p.y / bin_size).floor() as i64,
    )
}

// Main function to sort particles into bins in parallel
fn main() {
    let num_particles = 1000000;
    let bin_size = 10.0;

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
    // Create a global hashtable with pre-allocated capacity
    let mut global_bins: HashMap<(i64, i64), Vec<Particle>> = HashMap::with_capacity(num_particles / 10);

    // Process particles
    for particle in &particles {
        let bin_key = get_bin_key(particle, bin_size);
        global_bins.entry(bin_key).or_insert_with(Vec::new).push(particle.clone());
    }
    let duration_bin = start_bin.elapsed();
    println!("Time taken to bin particles: {:?}", duration_bin);
    /*
    // Timing the bin info printing
    let start_print = Instant::now();
    // Print some information about the bins
    for (key, bin) in global_bins.iter() {
        println!("Bin {:?} has {} particles", key, bin.len());
    }
    let duration_print = start_print.elapsed();
    println!("Time taken to print bin information: {:?}", duration_print);
    */
    // Total time
    let total_duration = start_gen.elapsed();
    println!("Total time taken: {:?}", total_duration);
}

