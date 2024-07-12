use rayon::prelude::*;
use hashbrown::HashMap;
use std::sync::Mutex;
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
    let num_particles = 1_000_000;
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
    // Create a global thread-safe hashtable
    let global_bins: Mutex<HashMap<(i64, i64), Vec<Particle>>> = Mutex::new(HashMap::new());
    let threads = 4;
    // Split particles into chunks and process in parallel
    particles.par_chunks(num_particles / threads) //rayon::current_num_threads())
        .for_each(|chunk| {
            let mut local_bins: HashMap<(i64, i64), Vec<Particle>> = HashMap::new();

            for particle in chunk {
                let bin_key = get_bin_key(particle, bin_size);
                local_bins.entry(bin_key).or_insert_with(Vec::new).push(particle.clone());
            }

            // Merge local bins into the global bin
            let mut global_bins_lock = global_bins.lock().unwrap();
            for (key, bin) in local_bins {
                global_bins_lock.entry(key).or_insert_with(Vec::new).extend(bin);
            }
            // 'global_bins_lock' goes out of scope here, and the mutex is automatically unlocked
        });
    let duration_bin = start_bin.elapsed();
    println!("Time taken to bin particles: {:?}", duration_bin);
    /*
    // Timing the bin info printing
    let start_print = Instant::now();
    // Print some information about the bins
    let global_bins_lock = global_bins.lock().unwrap();
    for (key, bin) in global_bins_lock.iter() {
        println!("Bin {:?} has {} particles", key, bin.len());
    }
    let duration_print = start_print.elapsed();
    println!("Time taken to print bin information: {:?}", duration_print);
    */
    // Total time
    let total_duration = start_gen.elapsed();
    println!("Total time taken: {:?}", total_duration);
}

