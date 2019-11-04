use resea::PAGE_SIZE;
use resea::idl;
use resea::error::Error;
use resea::channel::Channel;
use resea::message::{HandleId, Page};
use resea::std::cmp::min;
use resea::std::ptr;
use crate::page::PageAllocator;
use crate::initfs::{Initfs, File};
use crate::process::ProcessManager;

extern "C" {
    static __initfs: Initfs;
}

static KERNEL_SERVER: Channel = Channel::from_cid(1);

struct Server {
    ch: Channel,
    _initfs: &'static Initfs,
    process_manager: ProcessManager,
    page_allocator: PageAllocator,
}

const FREE_MEMORY_START: usize = 0x04000000;
const FREE_MEMORY_SIZE: usize  = 0x10000000;

impl Server {
    pub fn new(initfs: &'static Initfs) -> Server {
        Server {
            ch: Channel::create().unwrap(),
            _initfs: initfs,
            process_manager: ProcessManager::new(&KERNEL_SERVER),
            page_allocator: PageAllocator::new(FREE_MEMORY_START, FREE_MEMORY_SIZE),
        }
    }

    /// Launches all server executables in `startups` directory.
    pub fn launch_servers(&mut self, initfs: &Initfs) {
        let mut dir = initfs.open_dir();
        while let Some(file) = dir.readdir() {
            info!("initfs: path='{}', len={}KiB",
                  file.path(), file.len() / 1024);
            if file.path().starts_with("startups/") {
                self.execute(file).expect("failed to launch a server");
            }
        }
    }

    pub fn execute(&mut self, file: &'static File) -> Result<(), Error> {
        let pid = self.process_manager.create(file)?;
        let proc = self.process_manager.get(pid).unwrap();
        proc.pager_ch.transfer_to(&self.ch)?;
        proc.start(&KERNEL_SERVER)?;
        Ok(())
    }
}

impl idl::pager::Server for Server {
    fn fill(&mut self, pid: HandleId, addr: usize, num_pages: usize) -> Option<Result<Page, Error>> {
        // TODO: Support filling multiple pages.
        assert_eq!(num_pages, 1);

        // Search the process table. It should never fail.
        let proc = self.process_manager.get(pid).unwrap();

        // Look for the segment for `addr`.
        let mut page = self.page_allocator.allocate(1);
        let mut filled_page = false;
        let addr = addr as u64;
        let file_size = proc.file.len() as u64;
        for phdr in proc.elf.phdrs {
            if phdr.p_vaddr <= addr && addr < phdr.p_vaddr + phdr.p_memsz {
                let offset = addr - phdr.p_vaddr;
                let fileoff = phdr.p_offset + offset;
                if fileoff >= file_size {
                    // The file offset is beyond the file size. Ignore the
                    // segment for now.
                    continue;
                }

                // Found the appropriate segment. Fill the page with the file
                // contents.
                let copy_len = min(min(PAGE_SIZE as u64, file_size - fileoff),
                                   phdr.p_filesz - offset) as usize;
                unsafe {
                    let src = proc.file.data().as_ptr().add(fileoff as usize);
                    let dst = page.as_mut_ptr();
                    ptr::copy_nonoverlapping(src, dst, copy_len);
                }
                filled_page = true;
                break;
            }
        }

        if !filled_page {
            // `addr` is not in the ELF segments. It should be a zeroed pages
            // such as stack and heap. Fill the page with zeros.
            unsafe {
                ptr::write_bytes(page.as_mut_ptr(), 0, PAGE_SIZE);
            }
        }

        Some(Ok(page.as_page_payload()))
    }
}

impl idl::runtime::Server for Server {
    fn exit(&mut self, _code: i32) -> Option<Result<(), Error>> {
        unimplemented!();
    }

    fn printchar(&mut self, ch: u8) -> Option<Result<(), Error>> {
        resea::print::printchar(ch);
        Some(Ok(()))
    }
}

#[no_mangle]
pub fn main() {
    info!("hello from memmgr!");
    let initfs = unsafe { &__initfs };

    info!("lauching servers...");
    let mut server = Server::new(initfs);
    server.launch_servers(initfs);

    info!("entering mainloop...");
    serve_forever!(&mut server, [runtime, pager]);
}